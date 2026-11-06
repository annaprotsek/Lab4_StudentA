// Проєкт 2 (студент А): НЕЯВНЕ зв'язування з AnomalyLib партнера.
// Використовуються: anomaly.h (заголовок), AnomalyLib.lib (імпортна бібліотека), AnomalyLib.dll.
#include <iostream>
#include <limits>
#include <vector>

#include "anomaly.h"   // оголошення функцій з SDK партнера

static void show(const char* title, int status)
{
    std::cout << "  " << title << " -> status " << status << " (" << anomalyErrorMessage(status) << ")\n";
}

int main()
{
    std::cout << "=== IMPLICIT linking: AnomalyLib v" << anomalyVersion() << " ===\n\n";

    // ---------- 1. Нормальний сценарій ----------
    std::cout << "1. Normal usage\n";
    std::vector<double> data = { 10, 12, 11, 13, 12, 11, 10, 12, 13, 11, 12, 11, 100 };
    std::vector<int> flags(data.size());
    int count = 0;
    double rate = 0.0, lower = 0.0, upper = 0.0;

    int st = detectOutliers(data.data(), static_cast<int>(data.size()),
                            ANOMALY_METHOD_ZSCORE, 3.0, flags.data(), &count);
    show("detectOutliers(z-score, 3.0)", st);
    if (st == ANOMALY_OK)
    {
        std::cout << "     outliers: " << count << " -> indices:";
        for (size_t i = 0; i < flags.size(); ++i)
            if (flags[i]) std::cout << " " << i << " (value " << data[i] << ")";
        std::cout << "\n";
    }

    st = iqrOutliers(data.data(), static_cast<int>(data.size()), 1.5, flags.data(), &lower, &upper, &count);
    show("iqrOutliers(k = 1.5)", st);
    if (st == ANOMALY_OK)
        std::cout << "     bounds [" << lower << ", " << upper << "], outliers: " << count << "\n";

    st = anomalyRate(data.data(), static_cast<int>(data.size()), ANOMALY_METHOD_IQR, 1.5, &rate);
    show("anomalyRate(IQR)", st);
    if (st == ANOMALY_OK)
        std::cout << "     anomaly rate = " << rate << "\n";

    std::vector<double> z(data.size());
    st = zScore(data.data(), static_cast<int>(data.size()), z.data());
    show("zScore", st);
    if (st == ANOMALY_OK)
        std::cout << "     z[0] = " << z[0] << ", z[12] = " << z[12] << "\n";

    // ---------- 2. Граничні та некоректні дані (тестування бібліотеки партнера) ----------
    std::cout << "\n2. Edge cases of partner's library\n";
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    double few[4] = { 1, 2, 3, 4 };
    int f4[4] = {};
    double z4[4] = {};

    show("data == nullptr", detectOutliers(nullptr, 4, ANOMALY_METHOD_ZSCORE, 3.0, f4, &count));
    show("flags == nullptr", detectOutliers(few, 4, ANOMALY_METHOD_ZSCORE, 3.0, nullptr, &count));
    show("n == 0", detectOutliers(few, 0, ANOMALY_METHOD_ZSCORE, 3.0, f4, &count));
    show("n == 1", zScore(few, 1, z4));
    show("n == -5", detectOutliers(few, -5, ANOMALY_METHOD_IQR, 1.5, f4, &count));
    show("IQR with n == 3", detectOutliers(few, 3, ANOMALY_METHOD_IQR, 1.5, f4, &count));

    double withNan[4] = { 1, nan, 3, 4 };
    double withInf[4] = { 1, 2, inf, 4 };
    show("NaN in data", detectOutliers(withNan, 4, ANOMALY_METHOD_ZSCORE, 3.0, f4, &count));
    show("Inf in data", detectOutliers(withInf, 4, ANOMALY_METHOD_IQR, 1.5, f4, &count));

    double same[5] = { 7, 7, 7, 7, 7 };
    double z5[5] = {};
    show("all values equal (zScore)", zScore(same, 5, z5));

    show("unknown method (99)", detectOutliers(few, 4, 99, 3.0, f4, &count));
    show("threshold == 0", detectOutliers(few, 4, ANOMALY_METHOD_ZSCORE, 0.0, f4, &count));
    show("threshold < 0", detectOutliers(few, 4, ANOMALY_METHOD_ZSCORE, -2.0, f4, &count));
    show("threshold == NaN", detectOutliers(few, 4, ANOMALY_METHOD_IQR, nan, f4, &count));

    double huge[4] = { -1.7e308, 1.7e308, 1.7e308, -1.7e308 };
    show("values ~1e308", detectOutliers(huge, 4, ANOMALY_METHOD_ZSCORE, 3.0, f4, &count));

    // ---------- 3. Надвеликий масив ----------
    std::cout << "\n3. Large array (5 000 000 values)\n";
    const int N = 5000000;
    std::vector<double> big(N);
    std::vector<int> bigFlags(N);
    for (int i = 0; i < N; ++i)
        big[i] = static_cast<double>(i % 100);
    big[123456] = 1.0e6;

    st = detectOutliers(big.data(), N, ANOMALY_METHOD_ZSCORE, 3.0, bigFlags.data(), &count);
    show("z-score on 5M values", st);
    if (st == ANOMALY_OK)
        std::cout << "     outliers found: " << count << ", flags[123456] = " << bigFlags[123456] << "\n";

    std::cout << "\nDone. The program finished without crashing.\n";
    return 0;
}
