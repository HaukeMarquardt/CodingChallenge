#include "Planet.h"

Planet::Planet(string _name)
{
	Planet(_name, 0);
}

Planet::Planet(string _name, uint128_t _distanceToStart) : name(_name), distanceToStart(_distanceToStart)
{
}


Planet::~Planet()
{
}

// adds a possible route to another planet with distance (source -> target)
void Planet::addRoute(Planet *planet, uint128_t dist)
{
	// check if route already exists
	if (find(routes.begin(), routes.end(), make_pair(planet, dist)) != routes.end()) {
		return;
	}
	routes.push_back(make_pair(planet, dist));
}

// adds a possible route to another planet with distance (source -> target AND target -> source)
void Planet::addBidirectionalRoute(Planet *planet, uint128_t dist)
{
	addRoute(planet, dist);
	planet->addRoute(this, dist);
}