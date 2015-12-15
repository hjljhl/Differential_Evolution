#include <cstdlib>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <string>
#include <vector>
#include <cassert>
#include <utility>
#include <limits>
#include "Evolution.h"

using namespace std;
void gen_param(const vector<string>& names, const vector <double>& values, const string path)
{
    assert(names.size() == values.size());
    ofstream ofile;
    ofile.open(path);
    assert(ofile.is_open());
    for (size_t i = 0; i < names.size(); ++i)
    {
        ofile << ".param " << names[i] << " = " << values[i] << endl;
    }
    ofile.close();
}
double parse_ma0(const string path)
{
    ifstream ma0_file;
    ma0_file.open(path);
    if (!ma0_file.is_open())
    {
        cout << "fail to open " << path << endl;
        exit(EXIT_FAILURE);
    }
    string line;
    while (getline(ma0_file, line)) {} // we want the last line
    int idx1 = 0, idx2 = 0;
    while (line[idx1] == ' ')
    {
        idx1++;
        idx2 = idx1;
    }
    while (line[idx2] != ' ')
    {
        idx2 ++;
    }
    string gain_str = line.substr(idx1, idx2 - idx1);
    return atof(gain_str.c_str());
}
double run_spice(vector<double>& params)
{
    static size_t sim_num = 0; // count simulation number
    sim_num++;
    printf("Sim number: %ld\n", sim_num);
    vector<string> names
    {
        "cm"
        , "ival"
        , "l_fixed"
        , "w10"
        , "w11"
        , "w12"
        , "w13"
        , "w14"
        , "w15"
        , "w16_18"
        , "w17_19"
        , "w1_2"
        , "w20"
        , "w21"
        , "w22_23"
        , "w24"
        , "w25"
        , "w26"
        , "w27"
        , "w28"
        , "w29"
        , "w30"
        , "w31"
        , "w32"
        , "w33"
        , "w34"
        , "w35"
        , "w3_4"
        , "w5"
        , "w6"
        , "w7"
        , "w8"
        , "w9"
        , "vin_cm"
    };
    string para_path = "./circuit/param.sp";
    gen_param(names, params, para_path);
    system("cd circuit && hspice64 ./Single_ended.sp > output.info 2>&1");
    return parse_ma0("./circuit/Single_ended.ma0");
}
double opt_func(const vector<double>& params) // params without vin_cm
{
    // vector<double> sweep_vin_cm{0.3, 0.6, 0.9, 1.2, 1.5, 1.8, 2.1, 2.4, 2.7, 3.0};
    cout << "Here" << endl;
    vector<double> sweep_vin_cm{1.0};
    double gain = numeric_limits<double>::infinity();
    for(auto vin_cm : sweep_vin_cm)
    {
        vector<double> new_params = params;
        new_params.push_back(vin_cm);
        double this_gain = run_spice(new_params);
        if(this_gain < gain)
            gain = this_gain;
    }
    return gain;
}
int main()
{
    vector<pair<double, double>> ranges
    {
        make_pair(0.5, 10)
        , make_pair(0  , 3)
        , make_pair(0.35, 10)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
        , make_pair(0.4, 20)
    }; //without vin_cm
    const unsigned int iter_num = 1;
    const unsigned int para_num = ranges.size();
    DESolver desolver(opt_func, ranges, iter_num, para_num);
    vector<double> solution = desolver.solver();
    printf("Result is %g\n", opt_func(solution));
    return 0;
}
