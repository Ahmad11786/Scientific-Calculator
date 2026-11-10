#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    double firstNumber = 0;
    double lastAnswer = 0;
    double memoryValue = 0;

    QString pendingOperator;
    QString currentExpression;

    bool waitingForOperand = true;
    bool shiftActive = false;
    bool alphaActive = false;
    bool hypActive = false;
    bool degreeMode = true;

    void numberClicked();
    void decimal();
    void signButton();
    void clear();
    void remove();

    void operatorClicked();
    void calculate();
    void equalButton();
    void evaluateExpression();

    void scientificFunction();

    void shiftClicked();
    void alphaClicked();
    void hypClicked();
    void modeClicked();
    void setupClicked();

    void updateDisplay();
};

#endif
