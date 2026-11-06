// Локальний тест RegressionLib (етап 1 - статична бібліотека, етап 2 - перевірка DLL)
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

#include "regression.h"

static int g_failed = 0;

static void check(bool ok, const char* what)
{
    std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << what << "\n";
    if (!ok)
        ++g_failed;
}

static bool approx(double a, double b, double eps = 1e-9)
{
    return std::fabs(a - b) <= eps;
}

int main()
{
    std::cout << "RegressionLib version " << regressionVersion() << "\n\n";

    // ---- 1. Дані з відомим розв'язком (обчислено незалежно в NumPy) ----
    std::cout << "1. Typical data\n";
    const double x[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    const double y[8] = { 2.3, 4.1, 6.2, 7.9, 10.2, 11.8, 14.1, 16.0 };
    double slope = 0, intercept = 0, r2 = 0, p = 0;
    double res[8] = {};

    check(linearRegression(x, y, 8, &slope, &intercept) == REGRESSION_OK, "linearRegression returns OK");
    std::cout << "     slope = " << slope << ", intercept = " << intercept << "\n";
    check(approx(slope, 1.9642857142857142), "slope == 1.96428571...");
    check(approx(intercept, 0.23571428571428119), "intercept == 0.2357142857...");

    check(rSquared(x, y, 8, slope, intercept, &r2) == REGRESSION_OK, "rSquared returns OK");
    std::cout << "     R^2 = " << r2 << "\n";
    check(approx(r2, 0.9991280337160297), "R^2 == 0.99912803...");

    check(predict(slope, intercept, 10.0, &p) == REGRESSION_OK, "predict returns OK");
    check(approx(p, slope * 10.0 + intercept), "predict(10) == slope*10+intercept");

    check(residuals(x, y, 8, slope, intercept, res) == REGRESSION_OK, "residuals returns OK");
    double sum = 0;
    for (double e : res) sum += e;
    check(approx(sum, 0.0, 1e-9), "sum of residuals ~ 0");
    check(approx(res[0], 0.1, 1e-9), "residual[0] == 0.1");

    // ---- 2. Ідеальна пряма y = 3x + 2 ----
    std::cout << "\n2. Perfect line y = 3x + 2\n";
    const double xp[4] = { 0, 1, 2, 3 };
    const double yp[4] = { 2, 5, 8, 11 };
    check(linearRegression(xp, yp, 4, &slope, &intercept) == REGRESSION_OK, "OK");
    check(approx(slope, 3.0) && approx(intercept, 2.0), "slope == 3, intercept == 2");
    check(rSquared(xp, yp, 4, slope, intercept, &r2) == REGRESSION_OK && approx(r2, 1.0), "R^2 == 1");

    // ---- 3. Граничні та некоректні дані ----
    std::cout << "\n3. Edge cases\n";
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();

    slope = -777; intercept = -777;
    check(linearRegression(nullptr, y, 8, &slope, &intercept) == REGRESSION_ERR_NULL_POINTER, "x == nullptr");
    check(linearRegression(x, nullptr, 8, &slope, &intercept) == REGRESSION_ERR_NULL_POINTER, "y == nullptr");
    check(linearRegression(x, y, 8, nullptr, &intercept) == REGRESSION_ERR_NULL_POINTER, "slope == nullptr");
    check(linearRegression(x, y, 0, &slope, &intercept) == REGRESSION_ERR_INVALID_COUNT, "n == 0");
    check(linearRegression(x, y, 1, &slope, &intercept) == REGRESSION_ERR_INVALID_COUNT, "n == 1");
    check(linearRegression(x, y, -5, &slope, &intercept) == REGRESSION_ERR_INVALID_COUNT, "n < 0");

    const double xnan[3] = { 1, nan, 3 };
    const double yok[3] = { 1, 2, 3 };
    check(linearRegression(xnan, yok, 3, &slope, &intercept) == REGRESSION_ERR_INVALID_VALUE, "NaN in x");
    const double yinf[3] = { 1, inf, 3 };
    check(linearRegression(yok, yinf, 3, &slope, &intercept) == REGRESSION_ERR_INVALID_VALUE, "Inf in y");

    const double xsame[3] = { 5, 5, 5 };
    check(linearRegression(xsame, yok, 3, &slope, &intercept) == REGRESSION_ERR_DEGENERATE, "all x equal");
    const double xsame2[3] = { 0.1, 0.1, 0.1 };
    check(linearRegression(xsame2, yok, 3, &slope, &intercept) == REGRESSION_ERR_DEGENERATE, "all x == 0.1 (rounding safe)");

    const double xo[2] = { 0, 1 };
    const double yo[2] = { -1.7e308, 1.7e308 };
    check(linearRegression(xo, yo, 2, &slope, &intercept) == REGRESSION_ERR_OVERFLOW, "slope overflow");

    check(slope == -777 && intercept == -777, "outputs untouched after errors");

    check(rSquared(x, nullptr, 8, 1, 0, &r2) == REGRESSION_ERR_NULL_POINTER, "rSquared: y == nullptr");
    check(rSquared(x, y, 8, 1, 0, nullptr) == REGRESSION_ERR_NULL_POINTER, "rSquared: r2 == nullptr");
    const double yconst[3] = { 4, 4, 4 };
    check(rSquared(yok, yconst, 3, 0, 4, &r2) == REGRESSION_ERR_DEGENERATE, "rSquared: all y equal");
    check(rSquared(x, y, 8, nan, 0, &r2) == REGRESSION_ERR_INVALID_VALUE, "rSquared: slope == NaN");

    check(predict(1e308, 0, 10, &p) == REGRESSION_ERR_OVERFLOW, "predict overflow");
    check(predict(nan, 0, 1, &p) == REGRESSION_ERR_INVALID_VALUE, "predict: slope NaN");
    check(predict(1, 0, 1, nullptr) == REGRESSION_ERR_NULL_POINTER, "predict: result == nullptr");

    check(residuals(x, y, 8, 1, 0, nullptr) == REGRESSION_ERR_NULL_POINTER, "residuals: out == nullptr");
    check(residuals(x, y, 0, 1, 0, res) == REGRESSION_ERR_INVALID_COUNT, "residuals: n == 0");
    check(residuals(x, y, 1, 1, 0, res) == REGRESSION_OK, "residuals: n == 1 is allowed");

    // ---- 4. Великий масив ----
    std::cout << "\n4. Large array (5 000 000 points, y = 2x + 1)\n";
    const int N = 5000000;
    std::vector<double> bx(N), by(N), br(N);
    for (int i = 0; i < N; ++i)
    {
        bx[i] = i;
        by[i] = 2.0 * i + 1.0;
    }
    check(linearRegression(bx.data(), by.data(), N, &slope, &intercept) == REGRESSION_OK, "OK");
    check(approx(slope, 2.0, 1e-6) && approx(intercept, 1.0, 1e-3), "slope ~ 2, intercept ~ 1");
    check(residuals(bx.data(), by.data(), N, slope, intercept, br.data()) == REGRESSION_OK, "residuals OK");

    std::cout << "\n" << (g_failed == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED") << "\n";
    return g_failed == 0 ? 0 : 1;
}
