// Проєкт 3 (студент А): ЯВНЕ зв'язування з AnomalyLib партнера.
// НЕ використовується ні заголовковий файл, ні .lib - лише AnomalyLib.dll та README.md (контракт).
// Сигнатури функцій і значення кодів/констант узято з README.md партнера.
#include <windows.h>

#include <iostream>
#include <limits>
#include <vector>

// 1. Сигнатури функцій (з контракту). Конвенція виклику - __cdecl.
typedef int (__cdecl *ZScoreFn)(const double* data, int n, double* out);
typedef int (__cdecl *IqrOutliersFn)(const double* data, int n, double k, int* flags,
                                     double* lowerBound, double* upperBound, int* outlierCount);
typedef int (__cdecl *DetectOutliersFn)(const double* data, int n, int method, double threshold,
                                        int* flags, int* outlierCount);
typedef int (__cdecl *AnomalyRateFn)(const double* data, int n, int method, double threshold, double* rate);
typedef const char* (__cdecl *VersionFn)(void);
typedef const char* (__cdecl *ErrorMessageFn)(int status);

// Константи з контракту (заголовка немає, тому визначаємо самі)
static const int METHOD_ZSCORE = 0;
static const int METHOD_IQR = 1;

static ErrorMessageFn g_errorMessage = nullptr;

static void show(const char* title, int status)
{
    std::cout << "  " << title << " -> status " << status;
    if (g_errorMessage)
        std::cout << " (" << g_errorMessage(status) << ")";
    std::cout << "\n";
}

// Допоміжна функція: отримати адресу експортованої функції за ім'ям
template <typename Fn>
static Fn loadFunction(HMODULE lib, const char* name)
{
    FARPROC p = GetProcAddress(lib, name);
    if (p == nullptr)
        std::cerr << "ERROR: function '" << name << "' not found in DLL (GetLastError = "
                  << GetLastError() << ")\n";
    return reinterpret_cast<Fn>(p);
}

int main()
{
    std::cout << "=== EXPLICIT linking: AnomalyLib.dll ===\n\n";

    // 2. Завантажуємо DLL під час виконання програми
    HMODULE lib = LoadLibraryA("AnomalyLib.dll");
    if (lib == nullptr)
    {
        // 126 - файл не знайдено; 193 - розрядність DLL не збігається з розрядністю програми
        std::cerr << "ERROR: cannot load AnomalyLib.dll (GetLastError = " << GetLastError() << ")\n";
        return 1;
    }
    std::cout << "DLL is successfully loaded.\n";

    // 3. Отримуємо вказівники на функції за іменами
    auto zScore         = loadFunction<ZScoreFn>(lib, "zScore");
    auto iqrOutliers    = loadFunction<IqrOutliersFn>(lib, "iqrOutliers");
    auto detectOutliers = loadFunction<DetectOutliersFn>(lib, "detectOutliers");
    auto anomalyRate    = loadFunction<AnomalyRateFn>(lib, "anomalyRate");
    auto version        = loadFunction<VersionFn>(lib, "anomalyVersion");
    g_errorMessage      = loadFunction<ErrorMessageFn>(lib, "anomalyErrorMessage");

    if (!zScore || !iqrOutliers || !detectOutliers || !anomalyRate || !version || !g_errorMessage)
    {
        FreeLibrary(lib);
        return 1;
    }
    std::cout << "Library version: " << version() << "\n\n";

    // ---------- 1. Нормальний сценарій ----------
    std::cout << "1. Normal usage\n";
    std::vector<double> data = { 10, 12, 11, 13, 12, 11, 10, 12, 13, 11, 12, 11, 100 };
    const int n = static_cast<int>(data.size());
    std::vector<int> flags(data.size());
    int count = 0;
    double rate = 0.0, lower = 0.0, upper = 0.0;

    int st = detectOutliers(data.data(), n, METHOD_ZSCORE, 3.0, flags.data(), &count);
    show("detectOutliers(z-score, 3.0)", st);
    if (st == 0)
    {
        std::cout << "     outliers: " << count << " -> indices:";
        for (size_t i = 0; i < flags.size(); ++i)
            if (flags[i]) std::cout << " " << i << " (value " << data[i] << ")";
        std::cout << "\n";
    }

    st = iqrOutliers(data.data(), n, 1.5, flags.data(), &lower, &upper, &count);
    show("iqrOutliers(k = 1.5)", st);
    if (st == 0)
        std::cout << "     bounds [" << lower << ", " << upper << "], outliers: " << count << "\n";

    st = anomalyRate(data.data(), n, METHOD_IQR, 1.5, &rate);
    show("anomalyRate(IQR)", st);
    if (st == 0)
        std::cout << "     anomaly rate = " << rate << "\n";

    std::vector<double> z(data.size());
    st = zScore(data.data(), n, z.data());
    show("zScore", st);
    if (st == 0)
        std::cout << "     z[0] = " << z[0] << ", z[12] = " << z[12] << "\n";

    // ---------- 2. Граничні та некоректні дані ----------
    std::cout << "\n2. Edge cases of partner's library\n";
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    double few[4] = { 1, 2, 3, 4 };
    int f4[4] = {};
    double z4[4] = {};

    show("data == nullptr", detectOutliers(nullptr, 4, METHOD_ZSCORE, 3.0, f4, &count));
    show("flags == nullptr", detectOutliers(few, 4, METHOD_ZSCORE, 3.0, nullptr, &count));
    show("n == 0", detectOutliers(few, 0, METHOD_ZSCORE, 3.0, f4, &count));
    show("n == 1", zScore(few, 1, z4));
    show("n == -5", detectOutliers(few, -5, METHOD_IQR, 1.5, f4, &count));
    show("IQR with n == 3", detectOutliers(few, 3, METHOD_IQR, 1.5, f4, &count));

    double withNan[4] = { 1, nan, 3, 4 };
    double withInf[4] = { 1, 2, inf, 4 };
    show("NaN in data", detectOutliers(withNan, 4, METHOD_ZSCORE, 3.0, f4, &count));
    show("Inf in data", detectOutliers(withInf, 4, METHOD_IQR, 1.5, f4, &count));

    double same[5] = { 7, 7, 7, 7, 7 };
    double z5[5] = {};
    show("all values equal (zScore)", zScore(same, 5, z5));

    show("unknown method (99)", detectOutliers(few, 4, 99, 3.0, f4, &count));
    show("threshold == 0", detectOutliers(few, 4, METHOD_ZSCORE, 0.0, f4, &count));
    show("threshold < 0", detectOutliers(few, 4, METHOD_ZSCORE, -2.0, f4, &count));
    show("threshold == NaN", detectOutliers(few, 4, METHOD_IQR, nan, f4, &count));

    double huge[4] = { -1.7e308, 1.7e308, 1.7e308, -1.7e308 };
    show("values ~1e308", detectOutliers(huge, 4, METHOD_ZSCORE, 3.0, f4, &count));

    // ---------- 3. Надвеликий масив ----------
    std::cout << "\n3. Large array (5 000 000 values)\n";
    const int N = 5000000;
    std::vector<double> big(N);
    std::vector<int> bigFlags(N);
    for (int i = 0; i < N; ++i)
        big[i] = static_cast<double>(i % 100);
    big[123456] = 1.0e6;

    st = detectOutliers(big.data(), N, METHOD_ZSCORE, 3.0, bigFlags.data(), &count);
    show("z-score on 5M values", st);
    if (st == 0)
        std::cout << "     outliers found: " << count << ", flags[123456] = " << bigFlags[123456] << "\n";

    // 4. Вивантажуємо бібліотеку з пам'яті
    FreeLibrary(lib);
    std::cout << "\nDLL is unloaded. The program finished without crashing.\n";
    return 0;
}
