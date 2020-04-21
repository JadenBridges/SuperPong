#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void moveBall(int,int);
    void moveLeftPaddle(int);
    void moveRightPaddle(int);

private slots:
    void populateTextEdit();
    void spawnPowerup();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
