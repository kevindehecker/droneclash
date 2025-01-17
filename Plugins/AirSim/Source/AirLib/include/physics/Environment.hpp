// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef airsim_core_Environment_hpp
#define airsim_core_Environment_hpp

#include "common/Common.hpp"
#include "common/UpdatableObject.hpp"
#include "common/CommonStructs.hpp"
#include "common/EarthUtils.hpp"

namespace msr { namespace airlib {

class Environment : public UpdatableObject {
public:
    struct State {
        //these fields must be set at initialization time
        GeoPoint geo_point;
        real_T min_z_over_ground;
        Vector3r position;

        //these fields are computed
        Vector3r gravity;
        real_T air_pressure;
        real_T temperature;
        real_T air_density;

        State()
        {}
        State(const Vector3r& position_val, const GeoPoint& geo_point_val, real_T min_z_over_ground_val)
            : geo_point(geo_point_val), min_z_over_ground(min_z_over_ground_val), position(position_val)
        {
        }
    };
public:
    Environment()
    {
        //allow default constructor with later call for initialize
    }
    Environment(const State& initial)
    {
        initialize(initial);
    }
    void initialize(const State& initial)
    {
        initial_ = initial;

        home_geo_point_ = EarthUtils::HomeGeoPoint(initial_.geo_point);

        updateState(initial_, home_geo_point_);

        Environment::reset();
    }
    
    //in local NED coordinates
    void setPosition(const Vector3r& position)
    {
        current_.position = position;
    }

    const State& getInitialState() const
    {
        return initial_;
    }
    const State& getState() const
    {
        return current_;
    }
    State& getState()
    {
        return current_;
    }

    //*** Start: UpdatableState implementation ***//
    virtual void reset()
    {
        current_ = initial_;
    }

    virtual void update()
    {
        updateState(current_, home_geo_point_);
    }
    //*** End: UpdatableState implementation ***//

private:
    static void updateState(State& state, const EarthUtils::HomeGeoPoint& home_geo_point)
    {
        state.geo_point = EarthUtils::nedToGeodetic(state.position, home_geo_point);

        real_T geo_pot = EarthUtils::getGeopotential(state.geo_point.altitude / 1000.0f);
        state.temperature = EarthUtils::getStandardTemperature(geo_pot);
        state.air_pressure = EarthUtils::getStandardPressure(geo_pot, state.temperature);
        state.air_density = EarthUtils::getAirDensity(state.air_pressure, state.temperature);

        //TODO: avoid recalculating square roots
        state.gravity = Vector3r(0, 0, EarthUtils::getGravity(state.geo_point.altitude));
    }

private:
    State initial_, current_;
    EarthUtils::HomeGeoPoint home_geo_point_;
};

}} //namespace
#endif
