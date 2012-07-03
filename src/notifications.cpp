#include "notifications.h"

NotificationWidget::NotificationWidget(QSqlDatabase *db,
                                       QList<int> idFeedList,
                                       QList<int> cntNewNewsList, QWidget *parent) :
  QWidget(parent,  Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint),
  db_(db),
  idFeedList_(idFeedList),
  cntNewNewsList_(cntNewNewsList)
{
  setAttribute(Qt::WA_TranslucentBackground);
  setFocusPolicy(Qt::NoFocus);

  countShowNews_ = 5;

  iconTitle_ = new QLabel(this);
  iconTitle_->setPixmap(QPixmap(":/images/quiterss16"));
  textTitle_ = new QLabel(this);
  QFont fontTitle = textTitle_->font();
  fontTitle.setBold(true);
  textTitle_->setFont(fontTitle);

  closeButton_ = new QToolButton(this);
  closeButton_->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
  closeButton_->setStyleSheet(
        "QToolButton { border: none; padding: 0px; }"
//        "image: url(:/images/closeHover); }"
//        "QToolButton:hover {"
//        "image: url(:/images/closeHover); }"
        );

  QHBoxLayout *titleLayout = new QHBoxLayout();
  titleLayout->setMargin(5);
  titleLayout->setSpacing(5);
  titleLayout->addWidget(iconTitle_);
  titleLayout->addWidget(textTitle_, 1);
  titleLayout->addWidget(closeButton_);

  QWidget *titlePanel_ = new QWidget(this);
  titlePanel_->setObjectName("titleNotification");
  titlePanel_->setLayout(titleLayout);

  numPage_ = new QLabel(this);

  leftButton_ = new QToolButton(this);
  leftButton_->setIcon(QIcon(":/images/moveLeft"));
  leftButton_->setEnabled(false);
  rightButton_ = new QToolButton(this);
  rightButton_->setIcon(QIcon(":/images/moveRight"));

  QHBoxLayout *bottomLayout = new QHBoxLayout();
  bottomLayout->setMargin(2);
  bottomLayout->setSpacing(5);
  bottomLayout->addSpacing(3);
  bottomLayout->addWidget(numPage_);
  bottomLayout->addStretch(1);
  bottomLayout->addWidget(leftButton_);
  bottomLayout->addWidget(rightButton_);
  bottomLayout->addSpacing(3);

  QWidget *bottomPanel_ = new QWidget(this);
  bottomPanel_->setObjectName("bottomNotification");
  bottomPanel_->setLayout(bottomLayout);

  stackedWidget_ = new QStackedWidget(this);

  QVBoxLayout *pageLayout_ = new QVBoxLayout();
  pageLayout_->setSpacing(0);
  QWidget *pageWidget = new QWidget(this);
  pageWidget->setLayout(pageLayout_);
  stackedWidget_->addWidget(pageWidget);

  QVBoxLayout* mainLayout = new QVBoxLayout();
  mainLayout->setMargin(2);
  mainLayout->setSpacing(0);
  mainLayout->addWidget(titlePanel_);
  mainLayout->addWidget(stackedWidget_);
  mainLayout->addWidget(bottomPanel_);

  QWidget *mainWidget = new QWidget(this);
  mainWidget->setObjectName("notificationWidget");
  mainWidget->setLayout(mainLayout);

  QVBoxLayout* layout = new QVBoxLayout();
  layout->setMargin(0);
  layout->addWidget(mainWidget);

  setLayout(layout);

  int cntAllNews = 0;
  foreach (int cntNews, cntNewNewsList_) {
    cntAllNews = cntAllNews + cntNews;
  }
  textTitle_->setText(QString(tr("Incoming news: %1")).arg(cntAllNews));

  if (cntAllNews > countShowNews_) rightButton_->setEnabled(true);
  else rightButton_->setEnabled(false);

  QSqlQuery q(*db_);
  int cnt = 0;
  foreach (int idFeed, idFeedList_) {
    QString qStr = QString("SELECT text, image FROM feeds WHERE id=='%1'").
        arg(idFeed);
    q.exec(qStr);
    QString titleFeed;
    QPixmap iconTab;
    if (q.next()) {
      titleFeed = q.value(0).toString();
      QByteArray byteArray = q.value(1).toByteArray();
      if (!byteArray.isNull()) {
        iconTab.loadFromData(QByteArray::fromBase64(byteArray));
      } else {
        iconTab.load(":/images/feed");
      }
    }

    qStr = QString("SELECT title FROM news WHERE new=1 AND feedId=='%1'").
        arg(idFeed);
    q.exec(qStr);
    while (q.next()) {
      if (cnt >= countShowNews_) {
        cnt = 1;
        pageLayout_->setMargin(5);
        pageLayout_ = new QVBoxLayout();
        pageLayout_->setSpacing(0);
        QWidget *pageWidget = new QWidget(this);
        pageWidget->setLayout(pageLayout_);
        stackedWidget_->addWidget(pageWidget);
      } else cnt++;

      NewsItem *newsItem = new NewsItem(this);
      newsItem->iconNews->setPixmap(iconTab);
      newsItem->iconNews->setToolTip(titleFeed);

      QString titleStr = newsItem->titleNews->fontMetrics().elidedText(
            q.value(0).toString(), Qt::ElideRight, newsItem->titleNews->sizeHint().width());
      newsItem->titleNews->setText(titleStr);
      newsItem->titleNews->setToolTip(q.value(0).toString());
      pageLayout_->addWidget(newsItem);
    }
  }
  pageLayout_->addStretch(1);
  numPage_->setText(QString(tr("Page %1 of %2").arg("1").arg(stackedWidget_->count())));

  QPoint point(QApplication::desktop()->availableGeometry(0).width()-sizeHint().width(),
               QApplication::desktop()->availableGeometry(0).height()-sizeHint().height());
  move(point);
  pageLayout_->setMargin(5);

  showTimer_ = new QTimer(this);
  connect(showTimer_, SIGNAL(timeout()),
          this, SLOT(deleteLater()));
  connect(closeButton_, SIGNAL(clicked()),
          this, SLOT(deleteLater()));
  connect(leftButton_, SIGNAL(clicked()),
          this, SLOT(previousPage()));
  connect(rightButton_, SIGNAL(clicked()),
          this, SLOT(nextPage()));

//  showTimer_->start(10000);
}

void NotificationWidget::nextPage()
{
  stackedWidget_->setCurrentIndex(stackedWidget_->currentIndex()+1);
  if (stackedWidget_->currentIndex()+1 == stackedWidget_->count())
    rightButton_->setEnabled(false);
  if (stackedWidget_->currentIndex() != 0)
    leftButton_->setEnabled(true);
  numPage_->setText(QString(tr("Page %1 of %2").
                            arg(stackedWidget_->currentIndex()+1).
                            arg(stackedWidget_->count())));
}

void NotificationWidget::previousPage()
{
  stackedWidget_->setCurrentIndex(stackedWidget_->currentIndex()-1);
  if (stackedWidget_->currentIndex() == 0)
    leftButton_->setEnabled(false);
  if (stackedWidget_->currentIndex()+1 != stackedWidget_->count())
    rightButton_->setEnabled(true);
  numPage_->setText(QString(tr("Page %1 of %2").
                            arg(stackedWidget_->currentIndex()+1).
                            arg(stackedWidget_->count())));
}