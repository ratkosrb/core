/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 * Copyright (C) 2009-2011 MaNGOSZero <https://github.com/mangos/zero>
 * Copyright (C) 2011-2016 Nostalrius <https://nostalrius.org>
 * Copyright (C) 2016-2017 Elysium Project <https://github.com/elysium-project>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MANGOS_GRIDNOTIFIERSIMPL_H
#define MANGOS_GRIDNOTIFIERSIMPL_H

#include "GridNotifiers.h"
#include "WorldPacket.h"
#include "Corpse.h"
#include "Player.h"
#include "UpdateData.h"
#include "CreatureAI.h"
#include "SpellAuras.h"
#include "DBCStores.h"
#include "DBCEnums.h"
#include "Spell.h"
#include "SpellMgr.h"

template<class T>
inline void MaNGOS::VisibleNotifier::Visit(GridRefManager<T>& m)
{
    for(typename GridRefManager<T>::iterator iter = m.begin(); iter != m.end(); ++iter)
    {
        i_camera.UpdateVisibilityOf(iter->getSource(), i_data, i_visibleNow);
        i_clientGUIDs.erase(iter->getSource()->GetObjectGuid());
    }
}

inline void MaNGOS::ObjectUpdater::Visit(CreatureMapType& m)
{
    std::vector<Creature*> creaturesToUpdate;
    for (const auto& iter : m)
        creaturesToUpdate.push_back(iter.getSource());
    for (const auto& it : creaturesToUpdate)
    {
        WorldObject::UpdateHelper helper(it);
        helper.UpdateRealTime(i_now, i_timeDiff);
    }
}

inline void CallAIMoveLOS(Creature* c, Unit* moving)
{
    // Creature AI reaction
    if (!c->HasUnitState(UNIT_STAT_LOST_CONTROL | UNIT_STAT_IGNORE_MOVE_LOS) && !c->IsInEvadeMode() && c->AI())
    {
        bool alert = false;
        if (moving->IsVisibleForOrDetect(c, c, true, false, &alert))
              c->AI()->MoveInLineOfSight(moving);
        else
            if (moving->GetTypeId() == TYPEID_PLAYER && moving->HasStealthAura() && alert)
                c->AI()->TriggerAlert(moving);
    }
}

inline void PlayerCreatureRelocationWorker(Player* pl, Creature* c)
{
    CallAIMoveLOS(c, pl);
}

inline void CreatureCreatureRelocationWorker(Creature* c1, Creature* c2)
{
    CallAIMoveLOS(c1, c2);
    CallAIMoveLOS(c2, c1);
}

inline void MaNGOS::PlayerRelocationNotifier::Visit(CreatureMapType& m)
{
    if (!i_player.IsAlive() || i_player.IsTaxiFlying())
        return;

    for(auto & iter : m)
    {
        Creature* c = iter.getSource();
        if (c->IsAlive())
            PlayerCreatureRelocationWorker(&i_player, c);
    }
}

template<>
inline void MaNGOS::CreatureRelocationNotifier::Visit(PlayerMapType& m)
{
    if (!i_creature.IsAlive())
        return;

    for(auto & iter : m)
    {
        Player* player = iter.getSource();
        if (player->IsAlive() && !player->IsTaxiFlying())
            PlayerCreatureRelocationWorker(player, &i_creature);
    }
}

template<>
inline void MaNGOS::CreatureRelocationNotifier::Visit(CreatureMapType& m)
{
    if (!i_creature.IsAlive())
        return;

    for(auto & iter : m)
    {
        Creature* c = iter.getSource();
        if (c != &i_creature && c->IsAlive())
            CreatureCreatureRelocationWorker(c, &i_creature);
    }
}

inline void MaNGOS::DynamicObjectUpdater::VisitHelper(Unit* target)
{
    
}

template<>
inline void MaNGOS::DynamicObjectUpdater::Visit(CreatureMapType& m)
{

}

template<>
inline void MaNGOS::DynamicObjectUpdater::Visit(PlayerMapType& m)
{

}

// SEARCHERS & LIST SEARCHERS & WORKERS

// WorldObject searchers & workers

template<class Check>
void MaNGOS::WorldObjectSearcher<Check>::Visit(GameObjectMapType& m)
{
    // already found
    if (i_object)
        return;

    for(auto & itr : m)
    {
        if (i_check(itr.getSource()))
        {
            i_object = itr.getSource();
            return;
        }
    }
}

template<class Check>
void MaNGOS::WorldObjectSearcher<Check>::Visit(PlayerMapType& m)
{
    // already found
    if (i_object)
        return;

    for(auto & itr : m)
    {
        if (i_check(itr.getSource()))
        {
            i_object = itr.getSource();
            return;
        }
    }
}

template<class Check>
void MaNGOS::WorldObjectSearcher<Check>::Visit(CreatureMapType& m)
{
    // already found
    if (i_object)
        return;

    for(auto & itr : m)
    {
        if (i_check(itr.getSource()))
        {
            i_object = itr.getSource();
            return;
        }
    }
}

template<class Check>
void MaNGOS::WorldObjectSearcher<Check>::Visit(CorpseMapType& m)
{
    // already found
    if (i_object)
        return;

    for(auto & itr : m)
    {
        if (i_check(itr.getSource()))
        {
            i_object = itr.getSource();
            return;
        }
    }
}

template<class Check>
void MaNGOS::WorldObjectSearcher<Check>::Visit(DynamicObjectMapType& m)
{
    // already found
    if (i_object)
        return;

    for(auto & itr : m)
    {
        if (i_check(itr.getSource()))
        {
            i_object = itr.getSource();
            return;
        }
    }
}

template<class Check>
void MaNGOS::WorldObjectListSearcher<Check>::Visit(PlayerMapType& m)
{
    for(auto & itr : m)
        if (i_check(itr.getSource()))
            i_objects.push_back(itr.getSource());
}

template<class Check>
void MaNGOS::WorldObjectListSearcher<Check>::Visit(CreatureMapType& m)
{
    for(auto & itr : m)
        if (i_check(itr.getSource()))
            i_objects.push_back(itr.getSource());
}

template<class Check>
void MaNGOS::WorldObjectListSearcher<Check>::Visit(CorpseMapType& m)
{
    for(auto & itr : m)
        if (i_check(itr.getSource()))
            i_objects.push_back(itr.getSource());
}

template<class Check>
void MaNGOS::WorldObjectListSearcher<Check>::Visit(GameObjectMapType& m)
{
    for(auto & itr : m)
        if (i_check(itr.getSource()))
            i_objects.push_back(itr.getSource());
}

template<class Check>
void MaNGOS::WorldObjectListSearcher<Check>::Visit(DynamicObjectMapType& m)
{
    for(auto & itr : m)
        if (i_check(itr.getSource()))
            i_objects.push_back(itr.getSource());
}

// Gameobject searchers

template<class Check>
void MaNGOS::GameObjectSearcher<Check>::Visit(GameObjectMapType& m)
{
    // already found
    if (i_object)
        return;

    for(auto & itr : m)
    {
        if (i_check(itr.getSource()))
        {
            i_object = itr.getSource();
            return;
        }
    }
}

template<class Check>
void MaNGOS::GameObjectLastSearcher<Check>::Visit(GameObjectMapType& m)
{
    for(auto & itr : m)
    {
        if (i_check(itr.getSource()))
            i_object = itr.getSource();
    }
}

template<class Check>
void MaNGOS::GameObjectListSearcher<Check>::Visit(GameObjectMapType& m)
{
    for(auto & itr : m)
        if (i_check(itr.getSource()))
            i_objects.push_back(itr.getSource());
}

// Unit searchers

template<class Check>
void MaNGOS::UnitSearcher<Check>::Visit(CreatureMapType& m)
{
    // already found
    if (i_object)
        return;

    for(auto & itr : m)
    {
        if (i_check(itr.getSource()))
        {
            i_object = itr.getSource();
            return;
        }
    }
}

template<class Check>
void MaNGOS::UnitSearcher<Check>::Visit(PlayerMapType& m)
{
    // already found
    if (i_object)
        return;

    for(PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (i_check(itr->getSource()))
        {
            i_object = itr->getSource();
            return;
        }
    }
}

template<class Check>
void MaNGOS::UnitLastSearcher<Check>::Visit(CreatureMapType& m)
{
    for(auto & itr : m)
    {
        if (i_check(itr.getSource()))
            i_object = itr.getSource();
    }
}

template<class Check>
void MaNGOS::UnitLastSearcher<Check>::Visit(PlayerMapType& m)
{
    for(auto & itr : m)
    {
        if (i_check(itr.getSource()))
            i_object = itr.getSource();
    }
}

template<class Check>
void MaNGOS::UnitListSearcher<Check>::Visit(PlayerMapType& m)
{
    for(auto & itr : m)
        if (i_check(itr.getSource()))
            i_objects.push_back(itr.getSource());
}

template<class Check>
void MaNGOS::UnitListSearcher<Check>::Visit(CreatureMapType& m)
{
    for(auto & itr : m)
        if (i_check(itr.getSource()))
            i_objects.push_back(itr.getSource());
}

// Creature searchers

template<class Check>
void MaNGOS::CreatureSearcher<Check>::Visit(CreatureMapType& m)
{
    // already found
    if (i_object)
        return;

    for(CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (i_check(itr->getSource()))
        {
            i_object = itr->getSource();
            return;
        }
    }
}

template<class Check>
void MaNGOS::CreatureLastSearcher<Check>::Visit(CreatureMapType& m)
{
    for(auto & itr : m)
    {
        if (i_check(itr.getSource()))
            i_object = itr.getSource();
    }
}

template<class Check>
void MaNGOS::CreatureListSearcher<Check>::Visit(CreatureMapType& m)
{
    for(auto & itr : m)
        if (i_check(itr.getSource()))
            i_objects.push_back(itr.getSource());
}

template<class Check>
void MaNGOS::PlayerSearcher<Check>::Visit(PlayerMapType& m)
{
    // already found
    if (i_object)
        return;

    for(auto & itr : m)
    {
        if (i_check(itr.getSource()))
        {
            i_object = itr.getSource();
            return;
        }
    }
}

template<class Check>
void MaNGOS::PlayerLastSearcher<Check>::Visit(PlayerMapType& m)
{
    for (const auto& itr : m)
    {
        if (i_check(itr.getSource()))
            i_object = itr.getSource();
    }
}

template<class Check>
void MaNGOS::PlayerListSearcher<Check>::Visit(PlayerMapType& m)
{
    for(auto & itr : m)
        if (i_check(itr.getSource()))
            i_objects.push_back(itr.getSource());
}

template<class Builder>
void MaNGOS::LocalizedPacketDo<Builder>::operator()(Player* p)
{
    int32 loc_idx = p->GetSession()->GetSessionDbLocaleIndex();
    uint32 cache_idx = loc_idx + 1;

    // create if not cached yet
    if (i_data_cache.size() < cache_idx + 1 || !i_data_cache[cache_idx])
    {
        if (i_data_cache.size() < cache_idx + 1)
            i_data_cache.resize(cache_idx + 1);

        auto data = std::unique_ptr<WorldPacket>(new WorldPacket());

        i_builder(*data, loc_idx);

        i_data_cache[cache_idx] = std::move(data);
    }

    p->SendDirectMessage(i_data_cache[cache_idx].get());
}

template<class Builder>
void MaNGOS::LocalizedPacketListDo<Builder>::operator()(Player* p)
{
    int32 loc_idx = p->GetSession()->GetSessionDbLocaleIndex();
    uint32 cache_idx = loc_idx+1;
    WorldPacketList* data_list;

    // create if not cached yet
    if (i_data_cache.size() < cache_idx+1 || i_data_cache[cache_idx].empty())
    {
        if (i_data_cache.size() < cache_idx+1)
            i_data_cache.resize(cache_idx+1);

        data_list = &i_data_cache[cache_idx];

        i_builder(*data_list, loc_idx);
    }
    else
        data_list = &i_data_cache[cache_idx];

    for(auto & i : *data_list)
        p->SendDirectMessage(i);
}

#endif                                                      // MANGOS_GRIDNOTIFIERSIMPL_H
