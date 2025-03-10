/* Copyright (c) 2022-2023 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "xupdater.h"
#include "ui_xupdater.h"
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFile>
#include <QStandardPaths>  // For platform-independent file paths
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>

XUpdater::XUpdater(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::XUpdater)
    , networkManager(new QNetworkAccessManager(this))  // Initialize the network manager
{
    ui->setupUi(this);

    // Set window fixed size for all platforms
    setFixedSize(width(), height());

    // Initially hide the updating label
    ui->label->setVisible(false);

    // Set the initial progress bar value
    ui->progressBar->setValue(0);
    ui->progressBar->setRange(0, 100); // From 0 to 100%

    // Fetch release information from GitHub
    QUrl releaseUrl("https://api.github.com/repos/horsicq/DIE-engine/releases");
    QNetworkRequest request(releaseUrl);
    QNetworkReply* reply = networkManager->get(request);

    qDebug() << "Fetching release information from: " << releaseUrl.toString();

    // Connect the finished signal to handle the release information
    connect(reply, &QNetworkReply::finished, this, &XUpdater::handleReleaseInfo);
}

XUpdater::~XUpdater()
{
    delete ui;
    delete networkManager;
}

void XUpdater::updateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        int progress = static_cast<int>((bytesReceived * 100) / bytesTotal);
        ui->progressBar->setValue(progress);

        // Show the label when progress starts
        if (!ui->label->isVisible()) {
            ui->label->setVisible(true);
        }

        qDebug() << "Download progress: " << progress << "% (" << bytesReceived << " of " << bytesTotal << " bytes)";
    }
}

void XUpdater::fileDownloaded()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    if (reply->error() == QNetworkReply::NoError) {
        qDebug() << "Download completed successfully!";

        QByteArray fileData = reply->readAll();

        // Use QStandardPaths to save the file to a platform-independent location
        QString downloadLocation = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/die_portable.zip";

        QFile file(downloadLocation); // Save the ZIP file locally
        if (file.open(QIODevice::WriteOnly)) {
            file.write(fileData);
            file.close();
            qDebug() << "File saved to: " << downloadLocation;
        } else {
            qDebug() << "Failed to open file for writing: " << downloadLocation;
        }
    } else {
        qDebug() << "Download failed: " << reply->errorString();
    }

    reply->deleteLater();
}

void XUpdater::handleReleaseInfo()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        QJsonArray releases = jsonDoc.array();

        // Determine the OS and architecture
        QString osType = QSysInfo::productType();
        QString arch = QSysInfo::currentCpuArchitecture();
        qDebug() << "Detected OS:" << osType;
        qDebug() << "Detected architecture:" << arch;

        // Detect Ubuntu version if the OS is Linux
        QString ubuntuVersion;
        if (osType == "linux") {
            QProcess lsbRelease;
            lsbRelease.start("lsb_release", QStringList() << "-r" << "-s");
            lsbRelease.waitForFinished();
            ubuntuVersion = lsbRelease.readAll().trimmed();
            qDebug() << "Detected Ubuntu version:" << ubuntuVersion;
        }

        QString downloadLink;

        for (const QJsonValue &release : releases) {
            QJsonArray assets = release.toObject()["assets"].toArray();
            for (const QJsonValue &asset : assets) {
                QString assetName = asset.toObject()["name"].toString();
                qDebug() << "Checking asset:" << assetName;

                // Check for OS type and architecture
                if (osType == "windows" && assetName.contains("win") && assetName.contains("64")) {
                    downloadLink = asset.toObject()["browser_download_url"].toString();
                } else if (osType == "linux" && assetName.contains("lin") && assetName.contains(ubuntuVersion)) {
                    if ((arch == "x86_64" && assetName.contains("x86_64")) ||
                        (arch == "arm64" && assetName.contains("arm64"))) {
                        downloadLink = asset.toObject()["browser_download_url"].toString();
                    }
                } else if (osType == "osx" && assetName.contains("mac")) {
                    // Check for specific architectures (x86_64, arm64 for M1)
                    if ((arch == "x86_64" && assetName.contains("x86_64")) ||
                        (arch == "arm64" && assetName.contains("arm64"))) {
                        downloadLink = asset.toObject()["browser_download_url"].toString();
                    }
                }
                if (!downloadLink.isEmpty()) {
                    break;
                }
            }
            if (!downloadLink.isEmpty()) {
                break;
            }
        }

        if (!downloadLink.isEmpty()) {
            // Download the release
            QUrl url(downloadLink);
            QNetworkRequest request(url);
            QNetworkReply* downloadReply = networkManager->get(request);

            qDebug() << "Starting download from:" << url.toString();

            // Connect the download progress to the progress bar
            connect(downloadReply, &QNetworkReply::downloadProgress, this, &XUpdater::updateDownloadProgress);

            // Connect the finished signal to handle the downloaded file
            connect(downloadReply, &QNetworkReply::finished, this, &XUpdater::fileDownloaded);
        } else {
            qWarning() << "No suitable release found for the current OS and architecture";
        }
    } else {
        qWarning() << "Failed to fetch release info:" << qPrintable(reply->errorString());
    }

    reply->deleteLater();
}


