#include "screenshotwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QScreen>
#include <QGuiApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDateTime>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDesktopServices>

ScreenshotWidget::ScreenshotWidget(QWidget *parent)
    : QWidget(parent), selecting(false), selected(false), currentDrawMode(None), toolbar(nullptr)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);

    setupToolbar();

    // 创建尺寸标签
    sizeLabel = new QLabel(this);
    sizeLabel->setStyleSheet("QLabel { background-color: rgba(0, 0, 0, 180); color: white; "
                             "padding: 5px; border-radius: 3px; font-size: 12px; }");
    sizeLabel->hide();
}

ScreenshotWidget::~ScreenshotWidget()
{
}

void ScreenshotWidget::setupToolbar()
{
    toolbar = new QWidget(this);
    toolbar->setStyleSheet(
        "QWidget { background-color: rgba(40, 40, 40, 200); border-radius: 5px; }"
        "QPushButton { background-color: rgba(60, 60, 60, 255); color: white; "
        "border: none; padding: 8px 15px; border-radius: 3px; font-size: 13px; }"
        "QPushButton:hover { background-color: rgba(80, 80, 80, 255); }"
        "QPushButton:pressed { background-color: rgba(50, 50, 50, 255); }");

    QHBoxLayout *layout = new QHBoxLayout(toolbar);
    layout->setSpacing(5);
    layout->setContentsMargins(10, 5, 10, 5);

    // 绘制工具
    btnRect = new QPushButton("矩形", toolbar);
    btnArrow = new QPushButton("箭头", toolbar);
    btnText = new QPushButton("文字", toolbar);
    btnPen = new QPushButton("画笔", toolbar);

    // 操作按钮
    btnSave = new QPushButton("保存", toolbar);
    btnCopy = new QPushButton("复制", toolbar);
    btnCancel = new QPushButton("取消", toolbar);

    layout->addWidget(btnRect);
    layout->addWidget(btnArrow);
    layout->addWidget(btnText);
    layout->addWidget(btnPen);
    layout->addSpacing(10);
    layout->addWidget(btnSave);
    layout->addWidget(btnCopy);
    layout->addWidget(btnCancel);

    // 连接信号
    connect(btnSave, &QPushButton::clicked, this, &ScreenshotWidget::saveScreenshot);
    connect(btnCopy, &QPushButton::clicked, this, &ScreenshotWidget::copyToClipboard);
    connect(btnCancel, &QPushButton::clicked, this, &ScreenshotWidget::cancelCapture);

    connect(btnRect, &QPushButton::clicked, this, [this]()
            { currentDrawMode = Rectangle; });
    connect(btnArrow, &QPushButton::clicked, this, [this]()
            { currentDrawMode = Arrow; });
    connect(btnText, &QPushButton::clicked, this, [this]()
            { currentDrawMode = Text; });
    connect(btnPen, &QPushButton::clicked, this, [this]()
            { currentDrawMode = Pen; });

    toolbar->hide();
}

void ScreenshotWidget::startCapture()
{
    // 获取所有屏幕的截图
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen)
    {
        screenPixmap = screen->grabWindow(0);
    }

    // 设置窗口大小为屏幕大小
    setGeometry(screen->geometry());
    showFullScreen();

    selecting = false;
    selected = false;
    selectedRect = QRect();
}

void ScreenshotWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    // 绘制背景截图
    painter.drawPixmap(0, 0, screenPixmap);

    // 绘制半透明遮罩
    painter.fillRect(rect(), QColor(0, 0, 0, 100));

    // 如果有选中区域，显示选中区域的原始图像
    if (selecting || selected)
    {
        QRect currentRect;
        if (selecting)
        {
            currentRect = QRect(startPoint, endPoint).normalized();
        }
        else
        {
            currentRect = selectedRect;
        }

        if (!currentRect.isEmpty())
        {
            // 显示选中区域的原始图像
            painter.drawPixmap(currentRect, screenPixmap, currentRect);

            // 绘制选中框
            QPen pen(QColor(0, 150, 255), 2);
            painter.setPen(pen);
            painter.drawRect(currentRect);

            // 绘制尺寸信息
            QString sizeText = QString("%1 x %2").arg(currentRect.width()).arg(currentRect.height());
            sizeLabel->setText(sizeText);

            // 调整标签位置
            int labelX = currentRect.x();
            int labelY = currentRect.y() - sizeLabel->height() - 5;
            if (labelY < 0)
            {
                labelY = currentRect.y() + 5;
            }
            sizeLabel->move(labelX, labelY);
            sizeLabel->adjustSize();
            sizeLabel->show();

            // 绘制调整手柄
            if (selected)
            {
                int handleSize = 8;
                painter.setBrush(QColor(0, 150, 255));
                painter.setPen(Qt::white);

                // 四个角
                painter.drawRect(currentRect.left() - handleSize / 2, currentRect.top() - handleSize / 2,
                                 handleSize, handleSize);
                painter.drawRect(currentRect.right() - handleSize / 2, currentRect.top() - handleSize / 2,
                                 handleSize, handleSize);
                painter.drawRect(currentRect.left() - handleSize / 2, currentRect.bottom() - handleSize / 2,
                                 handleSize, handleSize);
                painter.drawRect(currentRect.right() - handleSize / 2, currentRect.bottom() - handleSize / 2,
                                 handleSize, handleSize);
            }
        }
    }
}

void ScreenshotWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        startPoint = event->pos();
        endPoint = event->pos();
        selecting = true;
        selected = false;
        toolbar->hide();
        update();
    }
}

void ScreenshotWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (selecting)
    {
        endPoint = event->pos();
        update();
    }
}

void ScreenshotWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && selecting)
    {
        selecting = false;
        selected = true;
        selectedRect = QRect(startPoint, endPoint).normalized();

        // 显示工具栏
        if (!selectedRect.isEmpty())
        {
            updateToolbarPosition();
            toolbar->show();
        }

        update();
    }
}

void ScreenshotWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        cancelCapture();
    }
    else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        if (selected && !selectedRect.isEmpty())
        {
            copyToClipboard();
        }
    }
}

void ScreenshotWidget::updateToolbarPosition()
{
    if (!selected || selectedRect.isEmpty())
    {
        return;
    }

    int toolbarWidth = toolbar->sizeHint().width();
    int toolbarHeight = toolbar->sizeHint().height();

    // 尝试将工具栏放在选中区域下方
    int x = selectedRect.x() + (selectedRect.width() - toolbarWidth) / 2;
    int y = selectedRect.bottom() + 10;

    // 如果超出屏幕底部，则放在选中区域上方
    if (y + toolbarHeight > height())
    {
        y = selectedRect.top() - toolbarHeight - 10;
    }

    // 确保不超出屏幕左右边界
    if (x < 10)
        x = 10;
    if (x + toolbarWidth > width() - 10)
    {
        x = width() - toolbarWidth - 10;
    }

    toolbar->move(x, y);
}

void ScreenshotWidget::saveScreenshot()
{
    if (!selected || selectedRect.isEmpty())
    {
        return;
    }

    // 从原始截图中裁剪选中区域
    QPixmap croppedPixmap = screenPixmap.copy(selectedRect);

    // 获取默认保存路径
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QString defaultFileName = defaultPath + "/screenshot_" +
                              QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".png";

    // 打开保存对话框
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "保存截图",
                                                    defaultFileName,
                                                    "PNG图片 (*.png);;JPEG图片 (*.jpg);;所有文件 (*.*)");

    if (!fileName.isEmpty())
    {
        if (croppedPixmap.save(fileName))
        {
            emit screenshotTaken();
            close();
        }
    }
}

void ScreenshotWidget::copyToClipboard()
{
    if (!selected || selectedRect.isEmpty())
    {
        return;
    }

    // 从原始截图中裁剪选中区域
    QPixmap croppedPixmap = screenPixmap.copy(selectedRect);

    // 复制到剪贴板
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setPixmap(croppedPixmap);

    emit screenshotTaken();
    close();
}

void ScreenshotWidget::cancelCapture()
{
    emit screenshotCancelled();
    close();
}
