#include <iostream>
#include <fstream>
#include <string>

#include "liars_deck.hpp"

using namespace std;

double compute_ev(LiarsNode& ln) {
    double ev = 0;
    double p_sum = 0;
    for (int x = 0; x <= 5; x++) {
        double p1 = p1_distr[x];
        double v = 0;
        for (int y = 0; y <= 5 && x + y <= 8; y++) {         
            double p2 = p2_distr[x][y];
           

            double result = ln.eval(x, y, x, y, false);
            ev += result * p1 * p2;
            p_sum += p1 * p2;
            v += result * p2;
        }
    }

    return ev;
}

double compute_exploitable_ev(LiarsNode& ln, int pnum) {
    double ev = 0;
    for (int x = 0; x <= 5; x++) {
        double p1 = p1_distr[x];

        vector<LiarsNode::RangeItem> vec;

        for (int y = 0; y <= 5 && x + y <= 8; y++) {
            double p2 = p2_distr[x][y];

            if (p2 > 0) {

                LiarsNode::RangeItem ri = {0};
                ri.current = y;
                ri.initial = y;
                ri.is_bluff = false;
                ri.p = p2;

                vec.push_back(ri);
            }
        }

        double result = ln.greedy_eval(pnum, vec, x, x);
        ev += result * p1;
    }

    return ev;
}

int getFirstNum(double r) {
    for (int x = 0; x < 6; x++) {
        r -= p1_distr[x];
        if (r <= 0) {
            return x;
        }
    }
    __debugbreak();
}

int getSecondNum(int first, double r) {
    for (int x = 0; x < 6; x++) {
        r -= p2_distr[first][x];
        if (r <= 0) {
            return x;
        }
    }
    __debugbreak();
}

int main() {

    mt19937 generator;
    random_device rd;
    generator.seed(rd());
    uniform_real_distribution<double> distribution;

    LiarsNode ln(0, 5, 5);

    for (int x = 1; x <= 100000000; x++) {    

        for (int y = 1; y >= 0; y--) {

            double rng1 = distribution(generator);
            double rng2 = distribution(generator);

            int first = getFirstNum(rng1);
            int second = getSecondNum(first, rng2);

            ln.cfr(first, second, y, first, second, false, x);
        }

        if (x % 1000000 == 0) {

            double ev = compute_ev(ln);
            double eev1 = compute_exploitable_ev(ln, 0); //Player 2 should have higher EV because they are exploiting P1
            double eev2 = compute_exploitable_ev(ln, 1); //Player 1 should have higher EV because they are exploiting P2

            cout << x << " EV: " << ev << " Exp: " << ((ev - eev1) + (eev2 - ev))/ 2.0 << endl;

            std::ofstream outFile("strategy.json");
            outFile << ln.to_json().dump(4);
            outFile.close();
        }

    }

    cout << "Done." << endl;
}