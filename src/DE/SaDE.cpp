#include "DE/SaDE.h"
#include "global.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <string>
#include <random>
#include <cassert>
#include <algorithm>
using namespace std;
SaDE::SaDE(Objective f, const Ranges& r, size_t np, size_t max_iter,
           SelectionStrategy ss, unordered_map<string, double> extra)
    : DE(f, r, nullptr, nullptr, nullptr, 0, 0, np, max_iter, extra),
      _strategy_pool(_init_strategy()), 
      _strategy_prob(_init_strategy_prob()), 
      _mem_success(deque<vector<size_t>>{}), 
      _mem_failure(deque<vector<size_t>>{}), 
      _crmemory(vector<deque<vector<double>>>(_strategy_pool.size()))
{
    vector<string> names{"lp", "fmu", "fsigma", "crmu", "crsigma"};
    for(auto n : names)
    {
        if(extra.find(n) == extra.end())
        {
            cerr << "Can't find " << n << " in extra conf" << endl;
            exit(EXIT_FAILURE);
        }
    }
    _fmu      = extra.find("fmu")->second;
    _fsigma   = extra.find("fsigma")->second;
    _crmu     = extra.find("crmu")->second;
    _crsigma  = extra.find("crsigma")->second;
    _lp       = static_cast<size_t>(extra.find("lp")->second);
    _selector = set_selector(ss, extra);
}
double SaDE::f() const noexcept
{
    return normal_distribution<double>(_fmu, _fsigma)(engine);
}
double SaDE::cr() const noexcept
{
    return _curr_cr;
}
vector<SaDE::Strategy> SaDE::_init_strategy() const noexcept
{
    return vector<Strategy>{Strategy(shared_ptr<IMutator>(new Mutator_Rand_1),
                                     shared_ptr<ICrossover>(new Crossover_Bin)),
                            Strategy(shared_ptr<IMutator>(new Mutator_Best_1),
                                     shared_ptr<ICrossover>(new Crossover_Bin)),
                            Strategy(shared_ptr<IMutator>(new Mutator_Rand_2),
                                     shared_ptr<ICrossover>(new Crossover_Bin)),
                            Strategy(shared_ptr<IMutator>(new Mutator_CurrentToRand_1),
                                     shared_ptr<ICrossover>(new Crossover_Exp)),
                            Strategy(shared_ptr<IMutator>(new Mutator_RandToBest_2),
                                     shared_ptr<ICrossover>(new Crossover_Bin))};
}
vector<double> SaDE::_init_strategy_prob() const noexcept
{
    const size_t num_strategy = _strategy_pool.size();
    return vector<double>(num_strategy, 1.0 / static_cast<double>(num_strategy));
}
void SaDE::_update_memory_prob(const std::vector<size_t>& strategy_vec,
                               const std::vector<Evaluated>& old_result,
                               const std::vector<Evaluated>& new_result) noexcept
{
    // gen new records
    vector<size_t> success_r(_strategy_pool.size(), 0);
    vector<size_t> failure_r(_strategy_pool.size(), 0);
    assert(old_result.size() == strategy_vec.size() && strategy_vec.size() == new_result.size());
    assert(_mem_success.size() == _mem_failure.size());
    for (size_t i = 0; i < strategy_vec.size(); ++i)
    {
        const size_t s_idx = strategy_vec[i];
        if (_selector->better(new_result[i], old_result[i]))
            ++success_r[s_idx];
        else
            ++failure_r[s_idx];
    }
    // update memory
    if (accumulate(success_r.begin(), success_r.end(), 0) != 0)
    {
        _mem_success.push_back(success_r);
        _mem_failure.push_back(failure_r);
    }
    if (_mem_success.size() > _lp)
    {
        _mem_success.pop_front();
        _mem_failure.pop_front();
        // update probablities
        const double epsilon = 0.01;  // to avoid null probablities
        vector<size_t> num_success(_strategy_pool.size(), 0);
        vector<size_t> num_failure(_strategy_pool.size(), 0);
        assert(_mem_success.size() == _lp && _lp == _mem_failure.size());
        for(size_t i = 0; i < _lp; ++i)
        {
            const vector<size_t>& rs = _mem_success[i];
            const vector<size_t>& rf = _mem_failure[i];
            assert(rs.size() == _strategy_pool.size() && _strategy_pool.size() == rf.size());
            for(size_t j = 0; j < _strategy_pool.size(); ++j)
            {
                num_success[j] += rs[j];
                num_failure[j] += rf[j];
            }
        }
        vector<double> success_rate(_strategy_pool.size(), epsilon);
        for (size_t i = 0; i < success_rate.size(); ++i)
        {
            if (num_success[i] + num_failure[i] > 0)
            {
                success_rate[i] += static_cast<double>(num_success[i]) /
                                   static_cast<double>(num_failure[i] + num_success[i]);
            }
        }
        const double prob_normalizer = accumulate(success_rate.begin(), success_rate.end(), 0.0);
        for (size_t i = 0; i < _strategy_prob.size(); ++i)
        {
            _strategy_prob[i] = success_rate[i] / prob_normalizer;
        }
    }
}
size_t SaDE::_select_strategy(const vector<double>& probs) const noexcept
{
    const double sum = accumulate(probs.begin(), probs.end(), 0.0);
    assert(fabs(sum - 1) < 0.01);  // probablities sum up to 1
    vector<pair<double, double>> ranges(probs.size(), {0, 0});
    for (size_t i = 0; i < probs.size(); ++i)
    {
        ranges[i].first  = i == 0 ? 0 : ranges[i - 1].second;
        ranges[i].second = i == 0 ? probs[i] : ranges[i].first + probs[i];
    }
    const double rand01 = uniform_real_distribution<double>(0, 1)(engine);
    size_t sampled = ranges.size();
    for (size_t i = 0; i < ranges.size(); ++i)
    {
        if (ranges[i].first <= rand01 && rand01 < ranges[i].second)
        {
            sampled = i;
            break;
        }
    }
    assert(sampled < ranges.size());
    return sampled;
}
vector<double> SaDE::gen_cr_vec(const std::vector<size_t>& s_vec) const noexcept
{
    auto random_func = [&](const double mu, const double sigma, const double lb, const double ub)->double{
        double tmp = lb - 1;
        while(tmp < lb || tmp > ub)
        {
            tmp = normal_distribution<double>(mu, sigma)(engine);
        }
        return tmp;
    };
    vector<double> cr_vec(_np, 0);
    if (_curr_gen <= _lp)
    {
        for (size_t i = 0; i < _np; ++i)
        {
            cr_vec[i] = random_func(_crmu, _crsigma, 0, 1);
        }
    }
    else
    {
        vector<double> crmu_vec(_strategy_pool.size(), 0);
        for (size_t i = 0; i < crmu_vec.size(); ++i)
        {
            vector<double> container;
            const auto& memory = _crmemory[i];
            assert(memory.size() == _lp);
            for (const auto& vec : memory)
            {
                for (auto val : vec)
                {
                    container.push_back(val);
                }
            }
            if (container.empty())
            {
                crmu_vec[i] = _crmu;
            }
            else
            {
                // median of all successfule CR in last LP generations
                // Why use median? why not mean?
                nth_element(container.begin(), container.begin() + container.size() / 2,
                            container.end());
                crmu_vec[i] = container[container.size() / 2];
            }
        }
        for (size_t i = 0; i < _np; ++i)
        {
            cr_vec[i] = random_func(crmu_vec[s_vec[i]], _crsigma, 0, 1);
        }
    }
    return cr_vec;
}
void SaDE::_update_cr_memory(const vector<size_t>& s_vec, const vector<double>& cr_vec,
                             const vector<Evaluated>& old_result,
                             const vector<Evaluated>& new_result) noexcept
{
    // really in-efficient!
    assert(s_vec.size() == _np && _np == cr_vec.size());
    assert(old_result.size() == _np && _np == new_result.size());
    assert(_crmemory.size() == _strategy_pool.size());
    for (size_t i = 0; i < _strategy_pool.size(); ++i)
    {
        _crmemory[i].push_back(vector<double>{});
        if (_curr_gen > _lp)
        {
            _crmemory[i].pop_front();
            assert(_crmemory[i].size() == _lp);
        }
    }
    for (size_t i = 0; i < cr_vec.size(); ++i)
    {
        const size_t s_idx = s_vec[i];
        if (_selector->better(new_result[i], old_result[i]))
        {
            _crmemory[s_idx][_crmemory[s_idx].size() - 1].push_back(cr_vec[i]);
        }
    }
}
Solution SaDE::solver()
{
    init();
    for (_curr_gen = 1; _curr_gen < _max_iter; ++_curr_gen)
    {
        vector<Solution> trials;
        vector<size_t> s_vec;
        trials.reserve(_np);
        s_vec.reserve(_np);
        for (size_t i = 0; i < _np; ++i)
        {
            s_vec.push_back(_select_strategy(_strategy_prob));
        }
        vector<double> cr_vec = gen_cr_vec(s_vec);
        assert(cr_vec.size() == _np);
        for (size_t i = 0; i < _np; ++i)
        {
            const Strategy& s = _strategy_pool[s_vec[i]];
            const Solution& target = _population[i];
            const Solution doner = s.mutator->mutation_solution(*this, i);
            _curr_cr = cr_vec[i];
            trials.push_back(s.crossover->crossover_solution(*this, target, doner));
        }
        vector<Evaluated> trial_results(_np);
#pragma omp parallel for
        // OpenMP 2.0 doesn't allow unsigned for loop index!
        // But it seems VS2015 is still using this version
        for (int p_idx = 0; p_idx < static_cast<int>(_population.size()); ++p_idx)
        {
            trial_results[p_idx] = _func(p_idx, trials[p_idx]);
        }
        _update_memory_prob(s_vec, _results, trial_results);
        _update_cr_memory(s_vec, cr_vec, _results, trial_results);
        auto new_result = _selector->select(*this, _population, trials, _results, trial_results);
        _results = new_result.first;
        _population = new_result.second;
        report_best();
    }
    size_t best_idx = find_best();
    return _population[best_idx];
}
