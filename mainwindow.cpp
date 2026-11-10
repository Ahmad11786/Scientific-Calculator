#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QStringList>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QtGlobal>

#include <cmath>
#include <cstdlib>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace {
constexpr double PI = 3.14159265358979323846;

// Small expression parser: supports + - * / ^, brackets, scientific
// notation, x, pi, e, and common mathematical functions.
class ExpressionParser
{
public:
    ExpressionParser(const QString &text, double xValue = 0.0,
                     bool degrees = true)
        : expression(text), x(xValue), degreeMode(degrees) {}

    double parse()
    {
        position = 0;
        const double value = parseExpression();
        skipSpaces();

        if (position != expression.size())
            throw std::runtime_error("Unexpected character in expression.");

        if (!std::isfinite(value))
            throw std::runtime_error("The result is outside the valid range.");

        return value;
    }

private:
    QString expression;
    int position = 0;
    double x = 0.0;
    bool degreeMode = true;

    void skipSpaces()
    {
        while (position < expression.size() &&
               expression.at(position).isSpace())
            ++position;
    }

    bool eat(QChar ch)
    {
        skipSpaces();
        if (position < expression.size() && expression.at(position) == ch) {
            ++position;
            return true;
        }
        return false;
    }

    double parseExpression()
    {
        double value = parseTerm();

        while (true) {
            if (eat('+'))
                value += parseTerm();
            else if (eat('-'))
                value -= parseTerm();
            else
                break;
        }
        return value;
    }

    double parseTerm()
    {
        double value = parsePower();

        while (true) {
            if (eat('*'))
                value *= parsePower();
            else if (eat('/')) {
                const double divisor = parsePower();
                if (divisor == 0.0)
                    throw std::runtime_error("Cannot divide by zero.");
                value /= divisor;
            } else {
                break;
            }
        }
        return value;
    }

    double parsePower()
    {
        double value = parseUnary();
        if (eat('^'))
            value = std::pow(value, parsePower());
        return value;
    }

    double parseUnary()
    {
        if (eat('+'))
            return parseUnary();
        if (eat('-'))
            return -parseUnary();
        return parsePrimary();
    }

    double parsePrimary()
    {
        skipSpaces();

        if (eat('(')) {
            const double value = parseExpression();
            if (!eat(')'))
                throw std::runtime_error("Missing closing bracket.");
            return value;
        }

        if (position >= expression.size())
            throw std::runtime_error("An operand is missing.");

        const QChar ch = expression.at(position);

        if (ch.isDigit() || ch == '.') {
            const int start = position;
            bool hasDigits = false;

            while (position < expression.size() &&
                   expression.at(position).isDigit()) {
                hasDigits = true;
                ++position;
            }

            if (position < expression.size() && expression.at(position) == '.') {
                ++position;
                while (position < expression.size() &&
                       expression.at(position).isDigit()) {
                    hasDigits = true;
                    ++position;
                }
            }

            if (!hasDigits)
                throw std::runtime_error("Invalid number.");

            if (position < expression.size() &&
                (expression.at(position) == 'e' ||
                 expression.at(position) == 'E')) {
                ++position;
                if (position < expression.size() &&
                    (expression.at(position) == '+' ||
                     expression.at(position) == '-'))
                    ++position;

                const int exponentStart = position;
                while (position < expression.size() &&
                       expression.at(position).isDigit())
                    ++position;

                if (position == exponentStart)
                    throw std::runtime_error("Invalid scientific notation.");
            }

            bool ok = false;
            const double value = expression.mid(start, position - start).toDouble(&ok);
            if (!ok)
                throw std::runtime_error("Invalid number.");
            return value;
        }

        if (ch.isLetter() || ch == QChar(0x03C0)) {
            const int start = position;
            while (position < expression.size() &&
                   (expression.at(position).isLetter() ||
                    expression.at(position) == QChar(0x03C0)))
                ++position;

            const QString name = expression.mid(start, position - start).toLower();

            if (name == "x") return x;
            if (name == "pi" || name == QString(QChar(0x03C0))) return PI;
            if (name == "e") return std::exp(1.0);

            if (!eat('('))
                throw std::runtime_error("A function needs parentheses.");

            const double argument = parseExpression();
            if (!eat(')'))
                throw std::runtime_error("Missing closing bracket.");

            const double angle = degreeMode ? argument * PI / 180.0 : argument;

            if (name == "sin") return std::sin(angle);
            if (name == "cos") return std::cos(angle);
            if (name == "tan") {
                if (std::fabs(std::cos(angle)) < 1e-12)
                    throw std::runtime_error("Tangent is undefined at this angle.");
                return std::tan(angle);
            }
            if (name == "sinh") return std::sinh(argument);
            if (name == "cosh") return std::cosh(argument);
            if (name == "tanh") return std::tanh(argument);
            if (name == "sqrt") {
                if (argument < 0) throw std::runtime_error("Square root needs a non-negative value.");
                return std::sqrt(argument);
            }
            if (name == "ln") {
                if (argument <= 0) throw std::runtime_error("Natural logarithm needs a positive value.");
                return std::log(argument);
            }
            if (name == "log") {
                if (argument <= 0) throw std::runtime_error("Logarithm needs a positive value.");
                return std::log10(argument);
            }
            if (name == "abs") return std::fabs(argument);

            throw std::runtime_error("Unknown function.");
        }

        throw std::runtime_error("Invalid expression.");
    }
};

QString cleanNumber(double value)
{
    return QString::number(value, 'g', 15);
}
}

// CONSTRUCTOR
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QPushButton *numbers[] = {
        ui->number0Button, ui->number1Button, ui->number2Button,
        ui->number3Button, ui->number4Button, ui->number5Button,
        ui->number6Button, ui->number7Button, ui->number8Button,
        ui->number9Button
    };

    for (QPushButton *button : numbers)
        connect(button, &QPushButton::clicked, this, &MainWindow::numberClicked);

    connect(ui->decimalButton, &QPushButton::clicked, this, &MainWindow::decimal);
    connect(ui->signButton, &QPushButton::clicked, this, &MainWindow::signButton);
    connect(ui->clearButton, &QPushButton::clicked, this, &MainWindow::clear);
    connect(ui->deleteButton, &QPushButton::clicked, this, &MainWindow::remove);

    QPushButton *operators[] = {
        ui->plusButton, ui->minusButton, ui->multiplyButton,
        ui->divideButton, ui->powerButton
    };
    for (QPushButton *button : operators)
        connect(button, &QPushButton::clicked, this, &MainWindow::operatorClicked);

    connect(ui->equalButton, &QPushButton::clicked, this, &MainWindow::equalButton);
    connect(ui->calcButton, &QPushButton::clicked, this, &MainWindow::evaluateExpression);

    QPushButton *scientificButtons[] = {
        ui->squareButton, ui->sqrtButton, ui->factorialButton,
        ui->reciprocalButton_2, ui->logButton, ui->lnButton,
        ui->lnSecondButton, ui->sinButton, ui->cosButton,
        ui->tanButton, ui->fractionButton, ui->angleButton,
        ui->logBaseButton, ui->integralButton
    };
    for (QPushButton *button : scientificButtons)
        connect(button, &QPushButton::clicked, this, &MainWindow::scientificFunction);

    connect(ui->shiftButton, &QPushButton::clicked, this, &MainWindow::shiftClicked);
    connect(ui->alphaButton, &QPushButton::clicked, this, &MainWindow::alphaClicked);
    connect(ui->hypButton, &QPushButton::clicked, this, &MainWindow::hypClicked);
    connect(ui->modeButton, &QPushButton::clicked, this, &MainWindow::modeClicked);
    connect(ui->setupButton, &QPushButton::clicked, this, &MainWindow::setupClicked);

    connect(ui->answerButton, &QPushButton::clicked, this, [this]() {
        currentExpression += cleanNumber(lastAnswer);
        waitingForOperand = false;
        updateDisplay();
    });

    connect(ui->expButton, &QPushButton::clicked, this, [this]() {
        // EXP starts a scientific-notation exponent, e.g. 1.2 EXP 3 = 1.2e3.
        if (currentExpression.isEmpty() ||
            currentExpression.endsWith('+') || currentExpression.endsWith('-') ||
            currentExpression.endsWith('*') || currentExpression.endsWith('/') ||
            currentExpression.endsWith('^') || currentExpression.endsWith('(')) {
            currentExpression += "1e";
        } else {
            currentExpression += "e";
        }
        waitingForOperand = false;
        updateDisplay();
    });

    connect(ui->leftBracketButton, &QPushButton::clicked, this, [this]() {
        currentExpression += "(";
        waitingForOperand = true;
        updateDisplay();
    });

    connect(ui->rightBracketButton, &QPushButton::clicked, this, [this]() {
        currentExpression += ")";
        waitingForOperand = false;
        updateDisplay();
    });

    connect(ui->memoryPlusButton, &QPushButton::clicked, this, [this]() {
        bool ok = false;
        const double value = ExpressionParser(currentExpression, 0, degreeMode).parse();
        Q_UNUSED(ok);
        if (alphaActive) {
            memoryValue -= value; // ALPHA + M+ acts as memory subtraction.
            alphaActive = false;
            ui->alphaButton->setStyleSheet("");
        } else {
            memoryValue += value;
        }
    });

    connect(ui->recallButton, &QPushButton::clicked, this, [this]() {
        if (alphaActive) { // ALPHA + RCL clears memory.
            memoryValue = 0;
            alphaActive = false;
            ui->alphaButton->setStyleSheet("");
            QMessageBox::information(this, "Memory", "Memory cleared.");
        } else {
            currentExpression = cleanNumber(memoryValue);
            waitingForOperand = false;
            updateDisplay();
        }
    });

    connect(ui->engineeringButton, &QPushButton::clicked, this, [this]() {
        try {
            const double value = ExpressionParser(currentExpression, 0, degreeMode).parse();
            if (value == 0) {
                currentExpression = "0";
            } else {
                const int exponent = static_cast<int>(
                    std::floor(std::log10(std::fabs(value)) / 3.0) * 3.0);
                const double mantissa = value / std::pow(10.0, exponent);
                currentExpression = cleanNumber(mantissa) + "e" + QString::number(exponent);
            }
            waitingForOperand = false;
            updateDisplay();
        } catch (const std::exception &error) {
            QMessageBox::warning(this, "Input Error", error.what());
        }
    });

    connect(ui->fractionDecimalButton, &QPushButton::clicked, this, [this]() {
        try {
            const double value = ExpressionParser(currentExpression, 0, degreeMode).parse();
            constexpr long long denominator = 1000000;
            const long long numerator = qRound64(value * denominator);
            const long long divisor = std::gcd(std::llabs(numerator), denominator);
            currentExpression = QString::number(numerator / divisor) + "/" +
                                QString::number(denominator / divisor);
            ui->expressionLabel->setText("Fraction");
            ui->resultLabel->setText(currentExpression);
            waitingForOperand = true;
        } catch (const std::exception &error) {
            QMessageBox::warning(this, "Input Error", error.what());
        }
    });

    ui->resultLabel->setText("0");
    ui->expressionLabel->setText("0");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateDisplay()
{
    ui->resultLabel->setText(currentExpression.isEmpty() ? "0" : currentExpression);
}

void MainWindow::numberClicked()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    if (!button) return;

    if (!currentExpression.isEmpty() &&
        (currentExpression.endsWith(')') || currentExpression.endsWith("pi") ||
         currentExpression.endsWith('e')) && waitingForOperand) {
        currentExpression += "*";
    }

    currentExpression += button->text();
    waitingForOperand = false;
    updateDisplay();
}

void MainWindow::decimal()
{
    // Add a decimal point only if the current numeric token has none.
    int start = currentExpression.size();
    while (start > 0) {
        const QChar c = currentExpression.at(start - 1);
        if (!(c.isDigit() || c == '.' || c == 'e' || c == 'E' ||
              ((c == '+' || c == '-') && start > 1 &&
               (currentExpression.at(start - 2) == 'e' ||
                currentExpression.at(start - 2) == 'E'))))
            break;
        --start;
    }

    const QString token = currentExpression.mid(start);
    if (!token.contains('.')) {
        currentExpression += "0.";
        if (!token.isEmpty() && token != "+" && token != "-")
            currentExpression.chop(2), currentExpression += token + ".";
    }
    waitingForOperand = false;
    updateDisplay();
}

void MainWindow::signButton()
{
    if (currentExpression.isEmpty()) {
        currentExpression = "-";
    } else {
        try {
            const double value = ExpressionParser(currentExpression, 0, degreeMode).parse();
            currentExpression = cleanNumber(-value);
        } catch (...) {
            if (currentExpression.startsWith('-'))
                currentExpression.remove(0, 1);
            else
                currentExpression.prepend('-');
        }
    }
    waitingForOperand = false;
    updateDisplay();
}

void MainWindow::clear()
{
    currentExpression.clear();
    pendingOperator.clear();
    firstNumber = 0;
    waitingForOperand = true;
    shiftActive = alphaActive = hypActive = false;
    ui->shiftButton->setStyleSheet("");
    ui->alphaButton->setStyleSheet("");
    ui->hypButton->setStyleSheet("");
    ui->expressionLabel->setText("0");
    updateDisplay();
}

void MainWindow::remove()
{
    if (!currentExpression.isEmpty())
        currentExpression.chop(1);
    waitingForOperand = false;
    updateDisplay();
}

void MainWindow::operatorClicked()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    if (!button) return;

    QString op = button->text();
    if (op == "×") op = "*";
    if (op == "÷") op = "/";

    if (currentExpression.isEmpty() && op != "-")
        return;

    if (!currentExpression.isEmpty()) {
        const QChar last = currentExpression.back();
        if (last == '+' || last == '-' || last == '*' || last == '/' || last == '^') {
            currentExpression.chop(1);
        }
    }

    currentExpression += op;
    pendingOperator = op;
    waitingForOperand = true;
    updateDisplay();
}

void MainWindow::calculate()
{
    evaluateExpression();
}

void MainWindow::equalButton()
{
    evaluateExpression();
}

void MainWindow::evaluateExpression()
{
    if (currentExpression.isEmpty()) return;

    try {
        const QString entered = currentExpression;
        const double result = ExpressionParser(entered, 0, degreeMode).parse();
        ui->expressionLabel->setText(entered + " =");
        currentExpression = cleanNumber(result);
        ui->resultLabel->setText(currentExpression);
        lastAnswer = result;
        firstNumber = result;
        pendingOperator.clear();
        waitingForOperand = true;
    } catch (const std::exception &error) {
        QMessageBox::warning(this, "Calculation Error", error.what());
    }
}

void MainWindow::scientificFunction()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    if (!button) return;

    const QString operation = button->text();
    double number = 0;

    try {
        number = ExpressionParser(currentExpression, 0, degreeMode).parse();
    } catch (const std::exception &error) {
        QMessageBox::warning(this, "Input Error", error.what());
        return;
    }

    const bool inverse = shiftActive;
    const bool hyperbolic = hypActive;
    double result = 0;

    if (operation == "∫") {
        bool ok = false;
        const QString function = QInputDialog::getText(
            this, "Numerical Integration", "Enter function in x (e.g. x^2 or sin(x)):",
            QLineEdit::Normal, "x^2", &ok);
        if (!ok || function.trimmed().isEmpty()) return;

        const double lower = QInputDialog::getDouble(
            this, "Lower Limit", "Enter lower limit:", 0, -1e9, 1e9, 6, &ok);
        if (!ok) return;
        const double upper = QInputDialog::getDouble(
            this, "Upper Limit", "Enter upper limit:", 1, -1e9, 1e9, 6, &ok);
        if (!ok) return;

        const int intervals = 10000; // Even number for Simpson's rule.
        const double h = (upper - lower) / intervals;
        try {
            double sum = ExpressionParser(function, lower, degreeMode).parse();
            sum += ExpressionParser(function, upper, degreeMode).parse();
            for (int i = 1; i < intervals; ++i) {
                const double x = lower + i * h;
                const double y = ExpressionParser(function, x, degreeMode).parse();
                sum += (i % 2 == 0 ? 2.0 : 4.0) * y;
            }
            result = sum * h / 3.0;
        } catch (const std::exception &error) {
            QMessageBox::warning(this, "Integration Error", error.what());
            return;
        }

        ui->expressionLabel->setText(
            QString("∫[%1,%2] %3 dx").arg(lower).arg(upper).arg(function));
        currentExpression = cleanNumber(result);
        ui->resultLabel->setText(currentExpression);
        lastAnswer = result;
        waitingForOperand = true;
        shiftActive = hypActive = false;
        ui->shiftButton->setStyleSheet("");
        ui->hypButton->setStyleSheet("");
        return;
    }

    if (operation == "sin") {
        if (inverse) {
            if (number < -1 || number > 1) {
                QMessageBox::warning(this, "Math Error", "Inverse sine accepts values from -1 to 1.");
                return;
            }
            result = std::asin(number) * (degreeMode ? 180.0 / PI : 1.0);
        } else if (hyperbolic) result = std::sinh(number);
        else result = std::sin(degreeMode ? number * PI / 180.0 : number);
    } else if (operation == "cos") {
        if (inverse) {
            if (number < -1 || number > 1) {
                QMessageBox::warning(this, "Math Error", "Inverse cosine accepts values from -1 to 1.");
                return;
            }
            result = std::acos(number) * (degreeMode ? 180.0 / PI : 1.0);
        } else if (hyperbolic) result = std::cosh(number);
        else result = std::cos(degreeMode ? number * PI / 180.0 : number);
    } else if (operation == "tan") {
        if (inverse) result = std::atan(number) * (degreeMode ? 180.0 / PI : 1.0);
        else if (hyperbolic) result = std::tanh(number);
        else {
            const double angle = degreeMode ? number * PI / 180.0 : number;
            if (std::fabs(std::cos(angle)) < 1e-12) {
                QMessageBox::warning(this, "Math Error", "Tangent is undefined at this angle.");
                return;
            }
            result = std::tan(angle);
        }
    } else if (operation == "x²") {
        result = inverse ? number * number * number : number * number;
    } else if (operation == "√") {
        if (inverse) result = std::cbrt(number);
        else if (number < 0) {
            QMessageBox::warning(this, "Math Error", "Square root needs a non-negative value.");
            return;
        } else result = std::sqrt(number);
    } else if (operation == "x⁻¹" || operation == "a/b") {
        if (number == 0) {
            QMessageBox::warning(this, "Math Error", "Zero has no reciprocal.");
            return;
        }
        result = 1.0 / number;
    } else if (operation == "x!") {
        if (number < 0 || number != std::floor(number) || number > 170) {
            QMessageBox::warning(this, "Math Error", "Factorial accepts whole numbers from 0 to 170.");
            return;
        }
        result = 1;
        for (int i = 2; i <= static_cast<int>(number); ++i) result *= i;
    } else if (operation == "log") {
        if (inverse) result = std::pow(10.0, number);
        else if (number > 0) result = std::log10(number);
        else {
            QMessageBox::warning(this, "Math Error", "Logarithm needs a positive number.");
            return;
        }
    } else if (operation == "ln") {
        if (inverse) result = std::exp(number);
        else if (number > 0) result = std::log(number);
        else {
            QMessageBox::warning(this, "Math Error", "Natural logarithm needs a positive number.");
            return;
        }
    } else if (operation == "logₐ") {
        bool ok = false;
        const double base = QInputDialog::getDouble(
            this, "Logarithm Base", "Enter base:", 2, 0.000001, 1000000, 6, &ok);
        if (!ok) return;
        if (base <= 0 || base == 1 || number <= 0) {
            QMessageBox::warning(this, "Math Error", "Base must be positive (not 1), and value must be positive.");
            return;
        }
        result = std::log(number) / std::log(base);
    } else if (operation == "° ′ ″") {
        const int degrees = static_cast<int>(number);
        const double minutesValue = std::fabs(number - degrees) * 60.0;
        const int minutes = static_cast<int>(minutesValue);
        const double seconds = (minutesValue - minutes) * 60.0;
        QMessageBox::information(this, "DMS",
                                 QString("%1° %2' %3\"").arg(degrees).arg(minutes).arg(seconds, 0, 'f', 3));
        return;
    } else if (operation == "a/b") {
        result = 1.0 / number;
    } else {
        return;
    }

    if (!std::isfinite(result)) {
        QMessageBox::warning(this, "Math Error", "This operation has no finite real result.");
        return;
    }

    QString label;
    if (inverse) label += "SHIFT ";
    if (hyperbolic) label += "HYP ";
    label += operation + "(" + cleanNumber(number) + ")";
    ui->expressionLabel->setText(label);
    currentExpression = cleanNumber(result);
    ui->resultLabel->setText(currentExpression);
    lastAnswer = result;
    waitingForOperand = true;

    shiftActive = false;
    hypActive = false;
    ui->shiftButton->setStyleSheet("");
    ui->hypButton->setStyleSheet("");
}

void MainWindow::shiftClicked()
{
    shiftActive = !shiftActive;
    ui->shiftButton->setStyleSheet(
        shiftActive ? "background-color: #F59E0B; color: black;" : "");
}

void MainWindow::alphaClicked()
{
    alphaActive = !alphaActive;
    ui->alphaButton->setStyleSheet(
        alphaActive ? "background-color: #22C55E; color: black;" : "");
}

void MainWindow::hypClicked()
{
    hypActive = !hypActive;
    ui->hypButton->setStyleSheet(
        hypActive ? "background-color: #38BDF8; color: black;" : "");
}

void MainWindow::modeClicked()
{
    degreeMode = !degreeMode;
    QMessageBox::information(this, "Angle Mode",
                             degreeMode ? "DEGREE MODE" : "RADIAN MODE");
}

void MainWindow::setupClicked()
{
    const QStringList choices = {"Degrees (DEG)", "Radians (RAD)"};
    bool ok = false;
    const QString selected = QInputDialog::getItem(
        this, "Calculator Setup", "Angle unit:", choices,
        degreeMode ? 0 : 1, false, &ok);
    if (!ok) return;

    degreeMode = (selected == choices.at(0));
    QMessageBox::information(this, "Setup",
                             degreeMode ? "Angle unit set to DEG." : "Angle unit set to RAD.");
}
