// regression.cpp - реалізація бібліотеки RegressionLib
#include "regression.h"

#include <cmath>

namespace
{
    bool allFinite(const double* a, int n)
    {
        for (int i = 0; i < n; ++i)
            if (!std::isfinite(a[i]))
                return false;
        return true;
    }

    bool allEqual(const double* a, int n)
    {
        for (int i = 1; i < n; ++i)
            if (a[i] != a[0])
                return false;
        return true;
    }

    // Перевірка пари масивів: nullptr -> кількість -> скінченність значень.
    int checkXY(const double* x, const double* y, int n, int minCount)
    {
        if (x == nullptr || y == nullptr)
            return REGRESSION_ERR_NULL_POINTER;
        if (n < minCount)
            return REGRESSION_ERR_INVALID_COUNT;
        if (!allFinite(x, n) || !allFinite(y, n))
            return REGRESSION_ERR_INVALID_VALUE;
        return REGRESSION_OK;
    }
}

REGRESSION_API int linearRegression(const double* x, const double* y, int n,
                                    double* slope, double* intercept)
{
    if (slope == nullptr || intercept == nullptr)
        return REGRESSION_ERR_NULL_POINTER;

    int st = checkXY(x, y, n, 2);
    if (st != REGRESSION_OK)
        return st;

    // Усі x однакові -> пряма не визначена (вертикальна).
    if (allEqual(x, n))
        return REGRESSION_ERR_DEGENERATE;

    double sumX = 0.0, sumY = 0.0;
    for (int i = 0; i < n; ++i)
    {
        sumX += x[i];
        sumY += y[i];
    }
    const double meanX = sumX / n;
    const double meanY = sumY / n;
    if (!std::isfinite(meanX) || !std::isfinite(meanY))
        return REGRESSION_ERR_OVERFLOW;

    // Двопрохідний алгоритм: стійкіший за формулу через суми x*x.
    double sxx = 0.0, sxy = 0.0;
    for (int i = 0; i < n; ++i)
    {
        const double dx = x[i] - meanX;
        const double dy = y[i] - meanY;
        sxx += dx * dx;
        sxy += dx * dy;
    }
    if (!std::isfinite(sxx) || !std::isfinite(sxy))
        return REGRESSION_ERR_OVERFLOW;
    if (sxx == 0.0)
        return REGRESSION_ERR_DEGENERATE;

    const double b = sxy / sxx;
    const double a = meanY - b * meanX;
    if (!std::isfinite(b) || !std::isfinite(a))
        return REGRESSION_ERR_OVERFLOW;

    *slope = b;      // вихідні параметри змінюються лише при успіху
    *intercept = a;
    return REGRESSION_OK;
}

REGRESSION_API int predict(double slope, double intercept, double x, double* result)
{
    if (result == nullptr)
        return REGRESSION_ERR_NULL_POINTER;
    if (!std::isfinite(slope) || !std::isfinite(intercept) || !std::isfinite(x))
        return REGRESSION_ERR_INVALID_VALUE;

    const double r = slope * x + intercept;
    if (!std::isfinite(r))
        return REGRESSION_ERR_OVERFLOW;

    *result = r;
    return REGRESSION_OK;
}

REGRESSION_API int rSquared(const double* x, const double* y, int n,
                            double slope, double intercept, double* r2)
{
    if (r2 == nullptr)
        return REGRESSION_ERR_NULL_POINTER;

    int st = checkXY(x, y, n, 2);
    if (st != REGRESSION_OK)
        return st;
    if (!std::isfinite(slope) || !std::isfinite(intercept))
        return REGRESSION_ERR_INVALID_VALUE;

    // Усі y однакові -> повна дисперсія 0, R^2 не визначено.
    if (allEqual(y, n))
        return REGRESSION_ERR_DEGENERATE;

    double sumY = 0.0;
    for (int i = 0; i < n; ++i)
        sumY += y[i];
    const double meanY = sumY / n;
    if (!std::isfinite(meanY))
        return REGRESSION_ERR_OVERFLOW;

    double ssRes = 0.0, ssTot = 0.0;
    for (int i = 0; i < n; ++i)
    {
        const double e = y[i] - (slope * x[i] + intercept);
        const double d = y[i] - meanY;
        ssRes += e * e;
        ssTot += d * d;
    }
    if (!std::isfinite(ssRes) || !std::isfinite(ssTot))
        return REGRESSION_ERR_OVERFLOW;
    if (ssTot == 0.0)
        return REGRESSION_ERR_DEGENERATE;

    const double r = 1.0 - ssRes / ssTot;
    if (!std::isfinite(r))
        return REGRESSION_ERR_OVERFLOW;

    *r2 = r;
    return REGRESSION_OK;
}

REGRESSION_API int residuals(const double* x, const double* y, int n,
                             double slope, double intercept, double* out)
{
    if (out == nullptr)
        return REGRESSION_ERR_NULL_POINTER;

    int st = checkXY(x, y, n, 1);
    if (st != REGRESSION_OK)
        return st;
    if (!std::isfinite(slope) || !std::isfinite(intercept))
        return REGRESSION_ERR_INVALID_VALUE;

    // Перший прохід: перевіряємо, що всі залишки скінченні (out ще не чіпаємо).
    for (int i = 0; i < n; ++i)
        if (!std::isfinite(y[i] - (slope * x[i] + intercept)))
            return REGRESSION_ERR_OVERFLOW;

    // Другий прохід: запис результату.
    for (int i = 0; i < n; ++i)
        out[i] = y[i] - (slope * x[i] + intercept);
    return REGRESSION_OK;
}

REGRESSION_API const char* regressionVersion(void)
{
    return "1.0.0";
}

REGRESSION_API const char* regressionErrorMessage(int status)
{
    switch (status)
    {
    case REGRESSION_OK:                return "OK";
    case REGRESSION_ERR_NULL_POINTER:  return "Null pointer argument";
    case REGRESSION_ERR_INVALID_COUNT: return "Invalid element count";
    case REGRESSION_ERR_INVALID_VALUE: return "Input contains NaN or Inf";
    case REGRESSION_ERR_DEGENERATE:    return "Degenerate data (no unique solution)";
    case REGRESSION_ERR_OVERFLOW:      return "Numeric overflow";
    default:                           return "Unknown status code";
    }
}
