/*
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
 */
/** @file LedgerInitializer.h
 *  @author chaychen
 *  @modify first draft
 *  @date 20181022
 */

#include "LedgerInitializer.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>

using namespace dev;
using namespace dev::initializer;

void LedgerInitializer::initConfig(boost::property_tree::ptree const& _pt)
{
    INITIALIZER_LOG(DEBUG) << LOG_BADGE("LedgerInitializer") << LOG_DESC("initConfig");
    m_groupDataDir = _pt.get<std::string>("group.group_data_path", "data/");
    assert(m_p2pService);
    /// TODO: modify FakeLedger to the real Ledger after all modules ready
    m_ledgerManager = std::make_shared<LedgerManager>(m_p2pService, m_keyPair);
    std::map<GROUP_ID, h512s> groudID2NodeList;
    bool succ = true;
    try
    {
        for (auto it : _pt.get_child("group"))
        {
            if (it.first.find("group_config.") != 0)
                continue;

            INITIALIZER_LOG(TRACE)
                << LOG_BADGE("LedgerInitializer") << LOG_DESC("load group config")
                << LOG_KV("groupID", it.first) << LOG_KV("config", it.second.data());
            std::vector<std::string> s;
            boost::split(s, it.first, boost::is_any_of("."), boost::token_compress_on);
            if (s.size() != 2)
            {
                INITIALIZER_LOG(ERROR)
                    << LOG_BADGE("LedgerInitializer") << LOG_DESC("parse groupID failed")
                    << LOG_KV("data", it.first.data());
                ERROR_OUTPUT << LOG_BADGE("LedgerInitializer") << LOG_DESC("parse groupID failed")
                             << std::endl;
                BOOST_THROW_EXCEPTION(MissingField());
            }

            succ =
                initSingleGroup(boost::lexical_cast<int>(s[1]), it.second.data(), groudID2NodeList);
            if (!succ)
            {
                INITIALIZER_LOG(ERROR)
                    << LOG_BADGE("LedgerInitializer") << LOG_DESC("initSingleGroup failed")
                    << LOG_KV("group", s[1]);
                ERROR_OUTPUT << LOG_BADGE("LedgerInitializer") << LOG_DESC("initSingleGroup failed")
                             << LOG_KV("group", s[1]) << std::endl;
                BOOST_THROW_EXCEPTION(InitLedgerConfigFailed());
            }
        }
        m_p2pService->setGroupID2NodeList(groudID2NodeList);
    }
    catch (std::exception& e)
    {
        INITIALIZER_LOG(ERROR) << LOG_BADGE("LedgerInitializer")
                               << LOG_DESC("parse group config faield")
                               << LOG_KV("EINFO", boost::diagnostic_information(e));
        ERROR_OUTPUT << LOG_BADGE("LedgerInitializer") << LOG_DESC("parse group config faield")
                     << LOG_KV("EINFO", boost::diagnostic_information(e)) << std::endl;
        BOOST_THROW_EXCEPTION(e);
    }
    /// stop the node if there is no group
    if (m_ledgerManager->getGrouplList().size() == 0)
    {
        INITIALIZER_LOG(ERROR) << LOG_BADGE("LedgerInitializer")
                               << LOG_DESC("Should init at least one group");
        BOOST_THROW_EXCEPTION(InitLedgerConfigFailed()
                              << errinfo_comment("[#LedgerInitializer]: Should init at least on "
                                                 "group! Please check configuration!"));
    }
}

bool LedgerInitializer::initSingleGroup(
    GROUP_ID _groupID, std::string const& _path, std::map<GROUP_ID, h512s>& _groudID2NodeList)
{
    bool succ = m_ledgerManager->initSingleLedger<Ledger>(_groupID, m_groupDataDir, _path);
    if (!succ)
    {
        return succ;
    }
    _groudID2NodeList[_groupID] =
        m_ledgerManager->getParamByGroupId(_groupID)->mutableConsensusParam().minerList;

    INITIALIZER_LOG(DEBUG) << LOG_BADGE("LedgerInitializer") << LOG_DESC("initSingleGroup")
                           << LOG_KV("groupID", std::to_string(_groupID));
    return succ;
}
