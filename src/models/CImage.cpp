#include "CImage.h"

#include "./exceptions/ImageNotSupportedException.h"
#include <QImageReader>
#include <QDir>
#include <QSettings>
#include <QTemporaryFile>
#include <cmath>


CImage::CImage(const QString& path)
{
    QFileInfo fileInfo = QFileInfo(path);
    auto* imageReader = new QImageReader(path);
    auto format = imageReader->format().toLower();

    if (!supportedFormats.contains(format)) {
        throw ImageNotSupportedException();
    }

    QSize imageSize = imageReader->size();

    this->fullPath = fileInfo.absoluteFilePath();
    this->fileName = fileInfo.fileName();
    this->size = fileInfo.size();
    this->compressedSize = this->size;

    this->width = imageSize.width();
    this->height = imageSize.height();
    this->compressedWidth = this->width;
    this->compressedHeight = this->height;

    this->status = CImageStatus::UNCOMPRESSED;

    delete imageReader;
}

bool operator==(const CImage& c1, const CImage& c2)
{
    return (c1.fullPath == c2.fullPath);
}

bool operator!=(const CImage& c1, const CImage& c2)
{
    return !(c1 == c2);
}

QString CImage::getFormattedSize()
{
    size_t size = this->status == CImageStatus::COMPRESSED ? this->compressedSize : this->size;
    return toHumanSize(size);
}

QString CImage::getRichFormattedSize()
{
    if (this->status == CImageStatus::COMPRESSED && this->size != this->compressedSize) {
        return "<small><s>" + toHumanSize(this->size) + "</s></small> " + toHumanSize(this->compressedSize);
    }
    return toHumanSize(this->size);
}

QString CImage::getResolution()
{
    if (this->status == CImageStatus::COMPRESSED) {
        return QString::number(this->compressedWidth) + "x" + QString::number(this->compressedHeight);
    }
    return QString::number(this->width) + "x" + QString::number(this->height);
}

QString CImage::getRichResolution()
{
    if (this->status == CImageStatus::COMPRESSED && (this->width != this->compressedWidth || this->height != this->compressedHeight)) {
        return "<small><s>" + QString::number(this->width) + "x" + QString::number(this->height) + "</s></small> " + QString::number(this->compressedWidth) + "x" + QString::number(this->compressedHeight);
    }
    return QString::number(this->width) + "x" + QString::number(this->height);
}

QString CImage::getFileName() const
{
    return fileName;
}

QString CImage::getFullPath() const
{
    return this->fullPath;
}

bool CImage::compress(CompressionOptions compressionOptions)
{
    QSettings settings;
    QString outputPath = compressionOptions.sameFolderAsInput ? QFileInfo(this->getFullPath()).absoluteDir().absolutePath() : compressionOptions.outputPath;
    QString inputFullPath = this->getFullPath();
    QString suffix = compressionOptions.suffix;
    QFileInfo inputFileInfo = QFileInfo(inputFullPath);
    QString fullFileName = inputFileInfo.completeBaseName() + suffix + "." + inputFileInfo.suffix();

    if (compressionOptions.keepStructure && !compressionOptions.sameFolderAsInput) {
        outputPath = inputFileInfo.absolutePath().remove(0, compressionOptions.basePath.length());
        outputPath = QDir::cleanPath(compressionOptions.outputPath + QDir::separator() + outputPath);
    }

    QDir outputDir(outputPath);
    if (!outputDir.exists()) {
        if (!outputDir.mkpath(outputPath)) {
            return false;
        }
    }

    QString outputFullPath = outputDir.absoluteFilePath(fullFileName);

    QString tempFileFullPath = "";
    bool outputAlreadyExists = QFile(outputFullPath).exists();

    QTemporaryFile tempFile;
    if (outputAlreadyExists || compressionOptions.resize) {
        if (tempFile.open()) {
            tempFileFullPath = tempFile.fileName();
        }

        if (tempFileFullPath.isEmpty()) {
            return false;
        }

        tempFile.close();
    }

    if (tempFileFullPath.isEmpty()) {
        tempFileFullPath = outputFullPath;
    }

    int compressionLevel = compressionOptions.compressionLevel;
    bool lossless = compressionOptions.lossless;
    bool keepMetadata = compressionOptions.keepMetadata;
    int res = 0;
    cs_image_pars compress_pars;

    if (compressionOptions.advancedMode) {
        compress_pars = initialize_parameters();
        compress_pars.jpeg.quality = compressionOptions.lossless ? 0 : compressionOptions.advancedJPEGPars.quality;
        compress_pars.jpeg.exif_copy = compressionOptions.advancedJPEGPars.exif_copy;

        compress_pars.png.iterations = compressionOptions.advancedPNGPars.iterations;
        compress_pars.png.iterations_large = compressionOptions.advancedPNGPars.iterations_large;
        compress_pars.png.lossy_8 = compressionOptions.advancedPNGPars.lossy_8;
        compress_pars.png.transparent = compressionOptions.advancedPNGPars.transparent;
    } else {
        compress_pars = getCompressionParametersFromLevel(compressionLevel, lossless, keepMetadata);
    }

    //Resize
    if (compressionOptions.resize) {
        QImage image(this->getFullPath());
        QSize originalSize = image.size();

        image = cResize(image,
            compressionOptions.fitTo,
            compressionOptions.width,
            compressionOptions.height,
            compressionOptions.size,
            compressionOptions.doNotEnlarge);

        if (image.size() != originalSize) {
            bool saveResult = image.save(tempFileFullPath, inputFileInfo.completeSuffix().toLocal8Bit(), 100);
            if (!saveResult) {
                return false;
            }
            inputFullPath = tempFileFullPath;
        }
    }

    bool result = cs_compress(inputFullPath.toUtf8().constData(), tempFileFullPath.toUtf8().constData(), &compress_pars, &res);
    if (result) {
        QFileInfo outputInfo(tempFileFullPath);
        if ((outputAlreadyExists && outputInfo.size() < inputFileInfo.size()) || compressionOptions.resize) {
            QFile::moveToTrash(outputFullPath);
            bool copyResult = QFile::copy(tempFileFullPath, outputFullPath);
            if (!copyResult) {
                return false;
            }
        }
        QFileInfo outputFileInfo = QFileInfo(outputFullPath);
        this->setCompressedInfo(outputFileInfo);
    }
    return result;
}

void CImage::setCompressedInfo(QFileInfo fileInfo)
{
    QImage compressedImage(fileInfo.absoluteFilePath());
    this->compressedSize = fileInfo.size();
    this->compressedFullPath = fileInfo.absoluteFilePath();
    this->compressedWidth = compressedImage.width();
    this->compressedHeight = compressedImage.height();
}

QString CImage::getCompressedFullPath() const
{
    return compressedFullPath;
}

double CImage::getRatio() const
{
    return (double)this->compressedSize / this->size;
}

QString CImage::getFormattedSavedRatio()
{
    return QString::number(round(100 - (this->getRatio() * 100))) + "%";
}

QString CImage::getRichFormattedSavedRatio()
{
    //TODO
    return this->getFormattedSavedRatio();
}

CImageStatus CImage::getStatus() const
{
    return status;
}

void CImage::setStatus(const CImageStatus& value)
{
    status = value;
}

size_t CImage::getOriginalSize() const
{
    return this->size;
}

size_t CImage::getCompressedSize() const
{
    return this->compressedSize;
}
