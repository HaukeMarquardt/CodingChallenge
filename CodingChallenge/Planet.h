#pragma once
#include <iostream>
#include <vector>
#include "uint128_t.h"

using namespace std;

class Planet
{
	public:
		// planet name and vector of pointers to other planets that are reachable
		// uint128_t represents the distance aka cost of that route
		string name;
		vector<pair<Planet*, uint128_t>> routes;

		// these variables are used for algorithm
		uint128_t distanceToStart;
		Planet *previousPlanet;

	public:
		Planet(string _name);
		Planet(string _name, uint128_t _distanceToStart);
		~Planet();
		void addRoute(Planet *planet, uint128_t dist);
		void addBidirectionalRoute(Planet *planet, uint128_t dist);
};