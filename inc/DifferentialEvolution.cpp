#include "DifferentialEvolution.h"
#include "util.h"
#include <iostream>
#include <random>
#include <omp.h>
#include <cassert>
#include <numeric>
#include <string>
using namespace std;
mt19937_64 engine(random_device{}());
double IMutator::boundary_constraint(pair<double, double> rg, double val) const noexcept
{
    assert(rg.first <= rg.second);
    uniform_real_distribution<double> distr(rg.first, rg.second);
    return rg.first <= val && val <= rg.second ? val : distr(engine);
}
vector<Solution> Mutator_Rand_1::mutation(const DE& de)
{
    const vector<Solution>& population = de.population();
    assert(population.size() == de.np());
    uniform_int_distribution<size_t> i_distr(0, population.size() - 1);
    vector<Solution> mutated = population;
    for (Solution& m : mutated)
    {
        size_t r1 = random_exclusive<size_t>(i_distr);
        size_t r2 = random_exclusive<size_t>(i_distr, vector<size_t> {r1});
        size_t r3 = random_exclusive<size_t>(i_distr, vector<size_t> {r1, r2});
        for (size_t i = 0; i < de.dimension(); ++i)
        {
            m[i] = population[r1][i] + de.f() * (population[r2][i] - population[r3][i]);
            m[i] = boundary_constraint(de.range(i), m[i]);
        }
    }
    return mutated;
}
vector<Solution> Mutator_Best_1::mutation(const DE& de)
{
    const vector<Solution>& population = de.population();
    assert(population.size() == de.np());
    uniform_int_distribution<size_t> i_distr(0, population.size() - 1);
    vector<Solution> mutated = population;
    size_t best_idx = de.find_best();
    for (Solution& m : mutated)
    {
        size_t r1 = random_exclusive<size_t>(i_distr, vector<size_t> {best_idx});
        size_t r2 = random_exclusive<size_t>(i_distr, vector<size_t> {best_idx, r1});
        for (size_t i = 0; i < de.dimension(); ++i)
        {
            m[i] = population[best_idx][i] + de.f() * (population[r1][i] - population[r2][i]);
            m[i] = boundary_constraint(de.range(i), m[i]);
        }
    }
    return mutated;
}
vector<Solution> Mutator_Best_2::mutation(const DE& de)
{
    const vector<Solution>& population = de.population();
    assert(population.size() == de.np());
    uniform_int_distribution<size_t> i_distr(0, population.size() - 1);
    vector<Solution> mutated = population;
    size_t best_idx = de.find_best();
    for (Solution& m : mutated)
    {
        size_t r1 = random_exclusive<size_t>(i_distr, vector<size_t> {best_idx});
        size_t r2 = random_exclusive<size_t>(i_distr, vector<size_t> {best_idx, r1});
        size_t r3 = random_exclusive<size_t>(i_distr, vector<size_t> {best_idx, r1, r2});
        size_t r4 = random_exclusive<size_t>(i_distr, vector<size_t> {best_idx, r1, r2, r3});
        for (size_t i = 0; i < de.dimension(); ++i)
        {
            m[i] = population[best_idx][i] + de.f() * (population[r1][i] - population[r2][i]) + de.f() * (population[r3][i] - population[r4][i]);
            m[i] = boundary_constraint(de.range(i), m[i]);
        }
    }
    return mutated;
}
vector<Solution> Mutator_RandToBest_1::mutation(const DE& de)
{
    const vector<Solution>& population = de.population();
    assert(population.size() == de.np());
    uniform_int_distribution<size_t> i_distr(0, population.size() - 1);
    vector<Solution> mutated = population;
    size_t best_idx = de.find_best();
    for (Solution& m : mutated)
    {
        size_t r1 = random_exclusive<size_t>(i_distr, vector<size_t> {best_idx});
        size_t r2 = random_exclusive<size_t>(i_distr, vector<size_t> {best_idx, r1});
        for (size_t i = 0; i < de.dimension(); ++i)
        {
            m[i] = m[i] + de.f() * (population[best_idx][i] - m[i]) + de.f() * (population[r1][i] - population[r2][i]);
            m[i] = boundary_constraint(de.range(i), m[i]);
        }
    }
    return mutated;
}
vector<Solution> Crossover_Bin::crossover(const DE& de, const vector<Solution>& targets, const vector<Solution>& doners)
{
    assert(targets.size() == de.np() && de.np() == doners.size());
    uniform_int_distribution<size_t> distr_j(0, de.dimension() - 1);
    uniform_real_distribution<double> distr_ij(0, 1);
    const double cr         = de.cr();
    const size_t dimension  = de.dimension();
    vector<Solution> trials = targets;
    for (size_t i = 0; i < de.np(); ++i)
    {
        assert(targets[i].size() == dimension && dimension == doners[i].size());
        const size_t jrand = distr_j(engine);
        for (size_t j = 0; j < dimension; ++j)
        {
            trials[i][j] = distr_ij(engine) <= cr || jrand == j ? doners[i][j] : targets[i][j];
        }
    };
    return trials;
}
vector<Solution> Crossover_Exp::crossover(const DE& de, const vector<Solution>& targets, const vector<Solution>& doners)
{
    assert(targets.size() == de.np() && de.np() == doners.size());
    const double cr         = de.cr();
    const size_t dim        = de.dimension();
    uniform_int_distribution<size_t>  int_distr(0, dim - 1);
    uniform_real_distribution<double> real_distr(0, 1);
    vector<Solution> trials = targets;
    for (size_t i = 0; i < de.np(); ++i)
    {
        assert(targets[i].size() == dim && dim == doners[i].size());
        size_t l = 1;
        for (; real_distr(engine) <= cr && l < dim; ++l) {}
        const size_t start_idx = int_distr(engine);
        for (size_t j = start_idx; j < start_idx + l; ++j)
        {
            trials[i][j%dim] = doners[i][j%dim];
        }
    }
    return trials;
}
pair<vector<Evaluated>, vector<Solution>> ISelector::select(const DE& de
                                       , const vector<Solution>& targets
                                       , const vector<Solution>& trials
                                       , const vector<Evaluated>& target_results
                                       , const vector<Evaluated>& trial_results)
{
    assert(targets.size() == de.np() && de.np() == trials.size());
    assert(target_results.size() == de.np() && de.np() == trial_results.size());
    vector<Solution> offspring(targets);
    vector<Evaluated> child_results(target_results);
    for (size_t i = 0; i < de.np(); ++i)
    {
        if (better(trial_results[i], target_results[i]))
        {
            copy(trials[i].begin(), trials[i].end(), offspring[i].begin());
            child_results[i] = trial_results[i];
        }
    }
    return make_pair(child_results, offspring);
}
pair<vector<Evaluated>, vector<Solution>> Selector_Epsilon::select(const DE& de
                                       , const vector<Solution>& targets
                                       , const vector<Solution>& trials
                                       , const vector<Evaluated>& target_results
                                       , const vector<Evaluated>& trial_results)
{
    if (de.curr_gen() == 1)
    {
        vector<double> violations(de.np(), numeric_limits<double>::infinity());
        for (size_t i = 0; i < target_results.size(); ++i)
            violations[i] = accumulate(target_results[i].second.begin(), target_results[i].second.end(), 0);
        size_t cutoff = (size_t)(de.np() * theta);
        partial_sort(violations.begin(), violations.begin() + cutoff, violations.end());
        epsilon_0     = violations[cutoff - 1];
        epsilon_level = epsilon_0;
    }
    auto ret      = ISelector::select(de, targets, trials, target_results, trial_results);
    size_t gen    = de.curr_gen();
    epsilon_level = gen > tc ? 0 : epsilon_0 * pow(1 - gen / tc, cp);
    return ret;
}
bool Selector_StaticPenalty::better(const Evaluated& r1, const Evaluated& r2)
{
    const double fom1 = r1.first + accumulate(r1.second.begin(), r1.second.end(), 0);
    const double fom2 = r2.first + accumulate(r2.second.begin(), r2.second.end(), 0);
    return fom1 <= fom2;
}
bool Selector_FeasibilityRule::better(const Evaluated& r1, const Evaluated& r2)
{
    const double fom1       = r1.first;
    const double fom2       = r2.first;
    const double violation1 = accumulate(r1.second.begin(), r1.second.end(), 0);
    const double violation2 = accumulate(r2.second.begin(), r2.second.end(), 0);
    if (violation1 == 0 && violation2 == 0)
        return fom1 <= fom2;
    else if (violation1 != 0 && violation2 != 0)
        return violation1 == violation2 ? fom1 <= fom2 : violation1 <= violation2;
    else
        return violation1 == 0;
}
bool Selector_Epsilon::better(const Evaluated& r1, const Evaluated& r2)
{
    const double fom1       = r1.first;
    const double fom2       = r2.first;
    const double violation1 = accumulate(r1.second.begin(), r1.second.end(), 0);
    const double violation2 = accumulate(r2.second.begin(), r2.second.end(), 0);
    if (violation1 <= epsilon_level && violation2 <= epsilon_level)
        return fom1 <= fom2;
    else if (violation1 > epsilon_level && violation2 > epsilon_level)
        return violation1 == violation2 ? fom1 <= fom2 : violation1 <= violation2;
    else
        return violation1 <= epsilon_level;
}
DE::DE(Objective func , const Ranges& rg
       , MutationStrategy ms , CrossoverStrategy cs , SelectionStrategy ss
       , double f , double cr , size_t np , size_t max_iter
       , unordered_map<string, double> extra)
    : _func(func), _ranges(rg), _f(f), _cr(cr), _np(np), _dim(rg.size()), _max_iter(max_iter), _extra_conf(extra),_curr_gen(0), _use_built_in_strategy(false)
{
    set_mutator(ms, extra);
    set_crossover(cs, extra);
    set_selector(ss, extra);
}
DE::DE(Objective func, const Ranges& rg
       , IMutator* m, ICrossover* c, ISelector* s
       , double f , double cr , size_t np , size_t max_iter
       , unordered_map<string, double> extra)
    : _func(func), _ranges(rg), _f(f), _cr(cr), _np(np), _dim(rg.size()), _max_iter(max_iter)
    , _extra_conf(extra),_curr_gen(0), _mutator(m), _crossover(c), _selector(s), _use_built_in_strategy(true)
{}
Solution DE::solver()
{
    init();
    report_best();
    for (_curr_gen = 1; _curr_gen < _max_iter; ++_curr_gen)
    {
        auto doners = _mutator->mutation(*this);
        auto trials = _crossover->crossover(*this, _population, doners);
        vector<Evaluated> trial_results(_np);
        transform(trials.begin(), trials.end(), trial_results.begin(), _func);
        auto new_result = _selector->select(*this, _population, trials, _results, trial_results);
        copy(new_result.first.begin(),  new_result.first.end(),  _results.begin());
        copy(new_result.second.begin(), new_result.second.end(), _population.begin());
        report_best();
    }
    size_t best_idx = find_best();
    return _population[best_idx];
}
DE::~DE()
{
    if (_use_built_in_strategy)
    {
        assert(_mutator != nullptr && _crossover != nullptr && _selector != nullptr);
        delete _mutator;
        delete _crossover;
        delete _selector;
    }
}
void DE::set_mutator(MutationStrategy ms, const unordered_map<string, double>& config)
{
    switch (ms)
    {
    case Rand1:
        _mutator = new Mutator_Rand_1;
        break;
    case Best1:
        _mutator = new Mutator_Best_1;
        break;
    case Best2:
        _mutator = new Mutator_Best_2;
        break;
    case RandToBest1:
        _mutator = new Mutator_RandToBest_1;
        break;
    default:
        _mutator = nullptr;
        cerr << "Unrecognoized Mutation Strategy" << endl;
        exit(EXIT_FAILURE);
    }
}
void DE::set_crossover(CrossoverStrategy cs, const unordered_map<string, double>& config)
{
    if (cs == CrossoverStrategy::Bin)
        _crossover = new Crossover_Bin;
    else if (cs == CrossoverStrategy::Exp)
        _crossover = new Crossover_Exp;
    else
    {
        _crossover = nullptr;
        cerr << "Unrecognoized Crossover Strategy" << endl;
        exit(EXIT_FAILURE);
    }
}
void DE::set_selector(SelectionStrategy ss, const unordered_map<string, double>& config)
{
    if (ss == SelectionStrategy::StaticPenalty)
        _selector = new Selector_StaticPenalty;
    else if (ss == SelectionStrategy::FeasibilityRule)
        _selector = new Selector_FeasibilityRule;
    else if (ss == SelectionStrategy::Epsilon)
    {
        for (string name : vector<string> {"theta", "tc", "cp"})
            assert(config.find(name) != config.end());
        double theta = config.find("theta")->second;
        size_t tc = config.find("tc")->second;
        size_t cp = config.find("cp")->second;
        _selector = new Selector_Epsilon(theta, tc, cp);
    }
    else
    {
        _crossover = nullptr;
        cerr << "Unrecognoized Selection Strategy" << endl;
        exit(EXIT_FAILURE);
    }
}
void DE::init()
{
    // rate of populations with non-infinity constraint violationss
    _population = vector<Solution>(_np, vector<double>(_dim, 0));
    _results    = vector<Evaluated>(_np);
    double theta = _extra_conf.find("theta") == _extra_conf.end() ? 0 : _extra_conf.find("theta")->second;
    if (theta < 0) theta = 0;
    if (theta > 1) theta = 1;
    size_t min_valid_num = (size_t)(_np * theta);
    vector<bool> valid(_np, false);
    size_t num_valid = 0;
    do
    {
        #pragma omp parallel for reduction(+:num_valid)
        for (size_t i = 0; i < _np; ++i)
        {
            if (! valid[i])
            {
                #pragma omp critical
                {
                    for (size_t j = 0; j < _dim; ++j)
                    {
                        double lb = _ranges.at(j).first;
                        double ub = _ranges.at(j).second;
                        uniform_real_distribution<double> distr(lb, ub);
                        _population[i][j] = distr(engine);
                    }
                }
                _results[i] = _func(_population[i]);
                bool valid_flag = true;
                for (auto vio : _results[i].second)
                {
                    if (std::isinf(vio)) valid_flag = false;
                }
                valid[i] = valid_flag;
                num_valid += valid_flag ? 1 : 0;
            }
        }
    }
    while (num_valid < min_valid_num);
    for(Solution p : _population)
    {
        for(double pp : p)
            cout << pp << ' ';
        cout << endl;
    }
}
size_t DE::find_best() const noexcept
{
    auto min_iter = min_element(_results.begin(), _results.end(), [&](const Evaluated & e1, const Evaluated & e2) -> bool
    {
        return _selector->better(e1, e2);
    });
    return distance(_results.begin(), min_iter);
}
void DE::report_best() const noexcept
{
    size_t best_idx = find_best();
    const Evaluated& best_result = _results[best_idx];
    const double violation = accumulate(best_result.second.begin(), best_result.second.end(), 0);
    cout << "Best idx: " << best_idx << ", Best FOM: " << best_result.first << ", Constraint Violation: " << violation << endl;
}