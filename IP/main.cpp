#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <chrono>
#include <omp.h>
#include <sciplot/sciplot.hpp>
using namespace sciplot;

const double max_x = 10.0;
const double min_x = -10.0;
const double max_y = 10.0;
const double min_y = -10.0;
// vienos gijos trukme - 10ms
// const int n = 20; // esamu parduotuvi skaicius
// const int m = 20; // planuojama pastatyti dar parduotuviu skaicius

// vienogs gijos trukme - 50ms
// const int n = 30; // esamu parduotuvi skaicius
// const int m = 30; // planuojama pastatyti dar parduotuviu skaicius

// vienos gijos trukme - 1400ms
// const int n = 50; // esamu parduotuvi skaicius
// const int m = 50; // planuojama pastatyti dar parduotuviu skaicius

// vienos gijos trukme - 4807ms
// const int n = 75; // esamu parduotuvi skaicius
// const int m = 75; // planuojama pastatyti dar parduotuviu skaicius

// vienos gijos trukme - 24841ms
// const int n = 100; // esamu parduotuvi skaicius
// const int m = 100; // planuojama pastatyti dar parduotuviu skaicius

// vienos gijos trukme - 18412ms
// const int n = 125; // esamu parduotuvi skaicius
// const int m = 125; // planuojama pastatyti dar parduotuviu skaicius

// vienos gijos trukme - 78481ms
// const int n = 135; // esamu parduotuvi skaicius
// const int m = 135; // planuojama pastatyti dar parduotuviu skaicius

// vienos gijos trukme - 154000ms, 4threads - 11270ms, 8threads - 5640ms, 10threads - 4000ms, 12threads - 3623ms, 14threads - 3316ms, 15threads - 3987, 16threads - 4268ms
int n; // = 150; // esamu parduotuvi skaicius
int m; // = 150; // planuojama pastatyti dar parduotuviu skaicius
const int threads = 15;

std::vector<double> x_n;
std::vector<double> y_n;
std::vector<double> x_m;
std::vector<double> y_m;

double atstumo_kaina_tarp_parduotuviu(double x1, double y1, double x2, double y2)
{
    return exp(-0.2 * (pow(x1 - x2, 2) + pow(y1 - y2, 2)));
}

std::pair<double, double> artimiausio_miesto_ribos_tasko_koordinates(double x1, double y1)
{
    double d1 = fabs(y1 - min_y);
    double d2 = fabs(y1 - max_y);
    double d3 = fabs(x1 - max_x);
    double d4 = fabs(x1 - min_x);

    if (d1 < d2 && d1 < d3 && d1 < d4)
    {
        return std::make_pair(x1, min_y);
    }
    else if (d2 < d1 && d2 < d3 && d2 < d4)
    {
        return std::make_pair(x1, max_y);
    }
    else if (d3 < d1 && d3 < d2 && d3 < d4)
    {
        return std::make_pair(max_x, y1);
    }
    else
    {
        return std::make_pair(min_x, y1);
    }
}

double atstumo_kaina_parduotuve_artimiausio_miesto_ribos_tasko(double x1, double y1)
{
    std::pair<double, double> koordinates = artimiausio_miesto_ribos_tasko_koordinates(x1, y1);

    // jeigu parduotuve planuojama statytis miesto ribose
    if (x1 >= min_x && x1 <= max_x && y1 >= min_y && y1 <= max_y)
    {
        return 0.0;
    }
    return exp(0.25 * (pow(x1 - koordinates.first, 2) + pow(y1 - koordinates.second, 2))) - 1.0;
}

double tikslas(std::vector<double> x_n, std::vector<double> y_n, std::vector<double> x_m, std::vector<double> y_m)
{
    double tikslas = 0.0;

#pragma omp parallel for reduction(+ : tikslas)
    for (int i = 0; i < m; i++)
    {
        // atstumo kaina iki jau pastatytu parduotuviu
        // #pragma omp parallel for reduction(+ : tikslas)
        for (int j = 0; j < n; j++)
        {
            double newTikslas = tikslas + atstumo_kaina_tarp_parduotuviu(x_m[i], y_m[i], x_n[j], y_n[j]);
            if (!std::isnan(newTikslas) && !std::isinf(newTikslas))
            {
                tikslas = newTikslas;
            }
        }

        // atstumo kaina iki kitu busimu parduotuviu
        // #pragma omp parallel for reduction(+ : tikslas)
        for (int j = 0; j < m; j++)
        {
            if (i != j)
            {
                double newTikslas = tikslas + atstumo_kaina_tarp_parduotuviu(x_m[i], y_m[i], x_m[j], y_m[j]);
                if (!std::isnan(newTikslas) && !std::isinf(newTikslas))
                {
                    tikslas = newTikslas;
                }
            }
        }

        // atstumo kaina iki artimiausio miesto ribos tasko
        double newTikslas = tikslas + atstumo_kaina_parduotuve_artimiausio_miesto_ribos_tasko(x_m[i], y_m[i]);
        if (!std::isnan(newTikslas) && !std::isinf(newTikslas))
        {
            tikslas = newTikslas;
        }

        // std::cout << "Tikslas: " << tikslas << std::endl;
    }
    return tikslas;
}

double linalg_norm(std::vector<double> &x, std::vector<double> &y)
{
    double norm = 0.0;

#pragma omp parallel for reduction(+ : norm)
    for (int i = 0; i < x.size(); i++)
    {
        norm += pow(x[i], 2) + pow(y[i], 2);
    }
    return sqrt(norm);
}

std::pair<std::vector<double>, std::vector<double>> gradient(std::vector<double> &x_m, std::vector<double> &y_m, std::vector<double> &x_n, std::vector<double> &y_n, double h)
{
    std::vector<double> gx(m, 0.0);
    std::vector<double> gy(m, 0.0);

    std::vector<double> x_m_h = x_m;
    std::vector<double> y_m_h = y_m;

#pragma omp parallel for
    for (int i = 0; i < m; i++)
    {
        x_m_h[i] += h;
        y_m_h[i] += h;
        gx[i] = (tikslas(x_m_h, y_m, x_n, y_n) - tikslas(x_m, y_m, x_n, y_n)) / h;
        gy[i] = (tikslas(x_m, y_m_h, x_n, y_n) - tikslas(x_m, y_m, x_n, y_n)) / h;
    }

    double norm = linalg_norm(gx, gy);

    std::pair<std::vector<double>, std::vector<double>> grad = std::make_pair(gx, gy);

#pragma omp parallel for
    for (int i = 0; i < m; i++)
    {
        grad.first[i] = gx[i] / norm;
        grad.second[i] = gy[i] / norm;
    }
    return grad;
}

void drawPlotAndShow(const std::string &title, int canvas_size, int line_width, std::vector<double> &x_n, std::vector<double> &y_n, std::vector<double> &x_m, std::vector<double> &y_m)
{
    Plot2D plot;
    plot.xlabel("x");
    plot.ylabel("y");
    constexpr double extra_range = 5; // additional range
    plot.xrange(min_x + min_x * extra_range, max_x + max_x * extra_range);
    plot.yrange(min_y + min_y * extra_range, max_y + max_y * extra_range);
    plot.legend().atOutsideBottom().displayHorizontal().displayExpandWidthBy(2);
    plot.grid().show();
    plot.drawDots(x_n, y_n).label("Esamos parduotuves").lineWidth(line_width);
    plot.drawDots(x_m, y_m).label("Planuojamos parduotuves").lineWidth(line_width);

    Figure fig = {{plot}};
    Canvas canvas = {{fig}};
    canvas.size(canvas_size, canvas_size);
    canvas.title(title);
    // canvas.show();
    canvas.save(title + ".png");
}

void generateData()
{
    // sugeneruoja atsitiktines koordinates
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis_x(min_x, max_x);
    std::uniform_real_distribution<> dis_y(min_y, max_y);
    for (int i = 0; i < n; i++)
    {
        x_n[i] = dis_x(gen);
        y_n[i] = dis_y(gen);
        std::cout << "x_n[" << i << "] = " << x_n[i] << std::endl;
    }

    for (int i = 0; i < m; i++)
    {
        x_m[i] = dis_x(gen);
        y_m[i] = dis_y(gen);
        std::cout << "x_m[" << i << "] = " << x_m[i] << std::endl;
    }

    // print the generated coordinates to 'duomenys_n20_m20.txt' file
    std::ofstream out("duomenys_n" + std::to_string(n) + "_m" + std::to_string(m) + ".txt");
    for (int i = 0; i < n; i++)
    {
        out << x_n[i] << " " << y_n[i] << std::endl;
    }
    for (int i = 0; i < m; i++)
    {
        out << x_m[i] << " " << y_m[i] << std::endl;
    }

    out.close();
}

void chooseDataSet(const int &dataSet)
{
    // vienos gijos trukme - 154000ms, 4threads - 11270ms, 8threads - 5640ms, 10threads - 4000ms, 12threads - 3623ms, 14threads - 3316ms, 15threads - 3987, 16threads - 4268ms
    switch (dataSet)
    {
    case 1: // vienos gijos trukme - 10ms
        n = 20;
        m = 20;
        break;
    case 2: // vienogs gijos trukme - 50ms
        n = 30;
        m = 30;
        break;
    case 3: // vienos gijos trukme - 1400ms
        n = 50;
        m = 50;

        break;
    case 4: // vienos gijos trukme - 4807ms
        n = 75;
        m = 75;
        break;
    case 5: // vienos gijos trukme - 24841ms
        n = 100;
        m = 100;
        break;
    case 6: // vienos gijos trukme - 18412ms
        n = 125;
        m = 125;
        break;
    case 7: // vienos gijos trukme - 78481ms
        n = 135;
        m = 135;
        break;
    case 8: // vienos gijos trukme - 154000ms,
        n = 150;
        m = 150;
        break;
    default:
        n = 20;
        m = 20;
    }

    x_n = std::vector<double>(n, 0.0);
    y_n = std::vector<double>(n, 0.0);
    x_m = std::vector<double>(m, 0.0);
    y_m = std::vector<double>(m, 0.0);

    // read data from file
    std::ifstream in("duomenys_n" + std::to_string(n) + "_m" + std::to_string(m) + ".txt");
    if (in.is_open())
    {
        std::cout << "Reading data...\n";
        for (int i = 0; i < n; i++)
        {
            in >> x_n[i] >> y_n[i];
        }
        for (int i = 0; i < m; i++)
        {
            in >> x_m[i] >> y_m[i];
        }
        in.close();
    }
    else
    {
        std::cout << "Unable to open file" << std::endl;
        generateData();
    }
}

int main()
{
    chooseDataSet(8);
    omp_set_dynamic(0);
    omp_set_num_threads(10);

    drawPlotAndShow("Pradines_koordinates_n" + std::to_string(n) + "_m" + std::to_string(m), 750, 10, x_n, y_n, x_m, y_m);

    double h = 0.01;
    int max_iter = 10000;
    double step = 1;
    double epsilon = (double)1e-15;

    auto start = std::chrono::high_resolution_clock::now();

    double prev_tikslo_reiksme = tikslas(x_m, y_m, x_n, y_n);

    for (int i = 0; i < max_iter; i++)
    {
        std::pair<std::vector<double>, std::vector<double>> grad = gradient(x_m, y_m, x_n, y_n, h);

        std::vector<double> x_m_h = x_m;
        std::vector<double> y_m_h = y_m;

#pragma omp parallel for
        for (int j = 0; j < m; j++)
        {
            x_m_h[j] -= step * grad.first[j];
            y_m_h[j] -= step * grad.second[j];
        }

        double tikslo_reiksme = tikslas(x_m_h, y_m_h, x_n, y_n);
        // std::cout << "ar yra NaN? " << std::isnan(tikslo_reiksme) << std::endl;
        if (std::isnan(tikslo_reiksme))
        {
            std::cout << "Nutraukiama." << std::endl;
            break;
        }

        if (tikslo_reiksme > prev_tikslo_reiksme)
        {
            // reset to previous values
            while (tikslo_reiksme > prev_tikslo_reiksme)
            {
                x_m_h = x_m;
                y_m_h = y_m;
                step /= 2;
// calculate the x_m_h and y_m_h again
#pragma omp parallel for
                for (int j = 0; j < m; j++)
                {
                    x_m_h[j] -= step * grad.first[j];
                    y_m_h[j] -= step * grad.second[j];
                }
                tikslo_reiksme = tikslas(x_m_h, y_m_h, x_n, y_n);

            }
        }
        else
        {
            step *= 1.1;
        }

        const double tikslu_skirtumas = fabs(tikslo_reiksme - prev_tikslo_reiksme);
        const double tikslumas = tikslu_skirtumas / (fabs(tikslo_reiksme) + fabs(prev_tikslo_reiksme));
        // if (tikslu_skirtumas < epsilon)
        //if (step < epsilon)
        std::cout << "Tikslumas: " << tikslumas << std::endl;
        if (tikslumas < epsilon)
        {
            std::cout << "Iteraciju skaicius: " << i << std::endl;
            std::cout << "Tikslumas: " << tikslumas << std::endl;
            break;
        }

        x_m = x_m_h;
        y_m = y_m_h;
        prev_tikslo_reiksme = tikslo_reiksme;
    }

    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "Laikas: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;

    std::cout << "Tikslas: " << prev_tikslo_reiksme << std::endl;
    std::cout << "Zingsnis: " << step << std::endl;
    drawPlotAndShow("Optimizuotos_koordinates_n" + std::to_string(n) + "_m" + std::to_string(m), 750, 10, x_n, y_n, x_m, y_m);

    return 0;
}
