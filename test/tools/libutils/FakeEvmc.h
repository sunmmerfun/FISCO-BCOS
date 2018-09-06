/**
 * @CopyRight:
 * FISCO-BCOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FISCO-BCOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FISCO-BCOS.  If not, see <http://www.gnu.org/licenses/>
 * (c) 2016-2018 fisco-dev contributors.
 *
 * @brief
 *
 * @file FakeEvmc.h
 * @author: jimmyshi
 * @date 2018-09-04
 */

#include <evmc/evmc.h>
#include <libdevcore/Address.h>
#include <libdevcore/FixedHash.h>
#include <libdevcore/SHA3.h>
#include <libethcore/EVMSchedule.h>
#include <libinterpreter/interpreter.h>
#include <test/tools/libutils/TestOutputHelper.h>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <map>
#include <memory>


namespace dev
{
namespace test
{
inline evmc_address toEvmC(Address const& _addr)
{
    return reinterpret_cast<evmc_address const&>(_addr);
}

inline evmc_uint256be toEvmC(h256 const& _h)
{
    return reinterpret_cast<evmc_uint256be const&>(_h);
}

inline u256 fromEvmC(evmc_uint256be const& _n)
{
    return fromBigEndian<u256>(_n.bytes);
}

inline Address fromEvmC(evmc_address const& _addr)
{
    return reinterpret_cast<Address const&>(_addr);
}

inline evmc_revision toRevision(dev::eth::EVMSchedule const& _schedule)
{
    if (_schedule.haveCreate2)
        return EVMC_CONSTANTINOPLE;
    if (_schedule.haveRevert)
        return EVMC_BYZANTIUM;
    if (_schedule.eip158Mode)
        return EVMC_SPURIOUS_DRAGON;
    if (_schedule.eip150Mode)
        return EVMC_TANGERINE_WHISTLE;
    if (_schedule.haveDelegateCall)
        return EVMC_HOMESTEAD;
    return EVMC_FRONTIER;
}

class FakeState
{
public:
    using AccountStateType = std::map<std::string const, evmc_uint256be>;
    using AccountCodeStateType = std::map<std::string const, bytes>;
    using StateType = std::map<std::string, AccountStateType>;

    void set(evmc_address const& addr, evmc_uint256be const& key, evmc_uint256be const& value)
    {
        state[toKey(addr)][toKey(key)] = value;  // create if the key is non-exist
    }

    evmc_uint256be& get(evmc_address const& addr, evmc_uint256be const& key)
    {
        return state[toKey(addr)][toKey(key)];
    }

    evmc_uint256be& accountBalance(evmc_address const& addr)
    {
        return state[toKey(addr)]["balance"];
    }

    bytes& accountCode(evmc_address const& addr) { return codeState[toKey(addr)]; }

    void accountSuicide(evmc_address const& addr)
    {
        state[toKey(addr)].clear();
        codeState[toKey(addr)].clear();
    }

    void printAccountAllData(evmc_address const& addr)
    {
        AccountStateType& accountState = state[toKey(addr)];
        for (auto& s : accountState)
        {
            std::cout << s.first << ":" << fromEvmC(s.second) << std::endl;
        }
    }

    bool addressExist(evmc_address const& addr) { return state[toKey(addr)].size() != 0; }

    bool valueExist(evmc_address const& addr, evmc_uint256be const& key)
    {
        AccountStateType& account = state[toKey(addr)];
        return account.find(toKey(key)) != account.end();
    }

    AccountStateType& operator[](std::string const& address) { return state[address]; }

private:
    inline std::string toKey(evmc_address const& addr) { return fromEvmC(addr).hex(); }

    inline std::string toKey(evmc_uint256be const& key)
    {
        return reinterpret_cast<h256 const&>(key).hex();
    }

    StateType state;
    AccountCodeStateType codeState;
};

class FakeEvmc
{
public:
    explicit FakeEvmc(evmc_instance* _instance);
    virtual ~FakeEvmc() { delete m_context; }

    evmc_result execute(dev::eth::EVMSchedule const& schedule, bytes code, bytes data,
        Address destination, Address caller, u256 value, int64_t gas, int32_t depth, bool isCreate,
        bool isStaticCall);

    void destroy();
    // TODO:Support more function

    FakeState& getState();

private:
    struct evmc_instance* m_instance;
    struct evmc_context* m_context;
};

}  // namespace test
}  // namespace dev
