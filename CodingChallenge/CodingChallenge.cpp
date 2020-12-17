/* CodingChallenge.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
	by Hauke Marquardt 05.2019

	Kommentare:
		-	Ich wandle die Distanzen (cost) in eine 128bit Festkommazahl um, da ein "double" die Werte aus der JSON Datei nicht präziese genug erfassen kann.
			Eine "long long" Variable würde zwar ausreichen um die Werte aus der Datei ohne Verluste zu erfassen, allerdings würde man bei der Addition der Werte schnell zu einem Überlauf kommen.
			Mir erschien die Variante mit einer uint128_t Zahl am einfachsten.

		-	Der Algorithmus mit seinen internen Sortierfunktonen arbeitet nur mit Pointern, dadurch erhoffe ich mir eine höhere Performance.

		-	SHOW_ALGORITHM_DEBUGGING kann genutzt werden um alternative / längere Stecken anzuzeigen.

		-	Als JSON Parser habe ich json11 von Dropbox, Inc verwendet. Für uint128_t Variablen die uint128_t Klasse von Jason Lee.

		-	Durchschnittliche interstellarAlgorithm(..,..) Dauer auf meinem Computer: ca. 2ms
*/

// debugging switch
//#define SHOW_ALGORITHM_DEBUGGING 

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <chrono>
#include "Planet.h"
#include "RSJparser.tcc"
#include "uint128_t.h"

RSJresource getJsonFromFile(string fileName);
uint128_t convertDistanceString(string distStr);
string distanceToString(uint128_t value);
Planet *getPlanetByName(string name);
vector<Planet*> interstellarAlgorithm(Planet *startPlanet, Planet*destinationPlanet);

// global variable for all planets
vector<Planet> planets;

int main()
{
	// welcome message
	cout << "# get in GmbH und Bertrandt Ingenieurbuero GmbH Coding-Challenge" << endl;
	cout << "# Einsendung von Hauke Marquardt" << endl << endl << endl;
	cout << "Parsing JSON file..." << endl;

	// load json file
	RSJresource data = getJsonFromFile("generatedGraph.json");
	// check if json file is valid
	if (!data["nodes"].exists()) {
		cout << "ERROR: JSON file does not exist!" << endl;
		system("pause");
		return 0;
	}

	// create highest 128bit integer (2^128-1)	closeToInf is used as initialization value for planet distances
	uint128_t closeToInf = 2;
	for (int i = 0; i < 126; i++)
		closeToInf *= 2;
	closeToInf += closeToInf - 1;

	// parse planets (nodes) and add them to global planets vector
	int i = 0;
	while (data["nodes"][i]["label"].exists()) {
		planets.push_back(Planet(data["nodes"][i]["label"].as<string>(), closeToInf));
		i++;
	}

	// parse routes (edges) and add them to corresponding planets, using i as iterator again
	i = 0;
	while (data["edges"][i].exists()) {
		// normalization only works with floating point values less than 1
		// so check if string starts with "0."
		if (data["edges"][i]["cost"].as<string>().rfind("0.", 0) == 0)
			planets[data["edges"][i]["source"].as<int>()].addBidirectionalRoute(&planets[data["edges"][i]["target"].as<int>()], convertDistanceString(data["edges"][i]["cost"].as<string>()));
		else
			cout << "WARNING: Distance is >= 1 (" << data["edges"][i]["cost"].as<string>() << ")" << endl;
		i++;
	}
	// show parsing result
	cout << "Loaded " << planets.size() << " planets and " << i << " routes from JSON File!" << endl << endl;

	// get start and destination planet for interstellarAlgorithm
	Planet	*startPlanet = getPlanetByName("Erde"),
			*destinationPlanet = getPlanetByName("b3-r7-r4nd7");

	// get precise time before algorithm execution
	chrono::high_resolution_clock::time_point time1 = chrono::high_resolution_clock::now();

	// execute algorithm and store result
	// The return value is a sorted list of waypoints
	vector<Planet*> tour = interstellarAlgorithm(startPlanet, destinationPlanet);

	// get precise time after algorithm execution
	chrono::high_resolution_clock::time_point time2 = chrono::high_resolution_clock::now();

	// output algorithm result in a text based table
	cout << "Shortest possible route \"" << tour.front()->name << "\" -> \"" << tour.back()->name << "\" Distance: " << distanceToString(tour.back()->distanceToStart) << endl << endl;
	cout << "\t\tPosition\t\t\tPlanet distance\t\t\t\tTotal distance traveled" << endl << endl;
	int stepCounter = 0;
	for (auto &node : tour) {
		if (stepCounter > 0)
			cout << "Step " << stepCounter << ":";
		if (node != startPlanet)
			cout << "\t\t" << node->name << " \t\t\t" << distanceToString(node->distanceToStart - node->previousPlanet->distanceToStart) << " \t\t\t" << distanceToString(node->distanceToStart) << endl;
		else
			cout << "\t\t" << node->name << endl;
		stepCounter++;
	}

	// calculate and show algorithm execution time 
	auto duration = chrono::duration_cast<chrono::microseconds>(time2 - time1).count();
	cout << endl << endl << "Interstellar navigation algorithm execution took: " << duration << "us" << endl;
	system("pause");
	return 1;
}

// reads file and returns RSJresource object
RSJresource getJsonFromFile(string fileName) {
	ifstream jsonFile(fileName);
	// check if file exists
	if (!jsonFile.fail()) 
		return RSJresource(jsonFile);
	return RSJresource("");
}

// sets all values to same width and converts to uint128_t (Normalization)
uint128_t convertDistanceString(string distStr) {
	string numerAsString = distStr;
	numerAsString.erase(0, 2);
	numerAsString.append(19 - numerAsString.length(), '0');
	return (uint128_t)stoull(numerAsString);
}

// reconverts uint128_t representation of distance to readable string
string distanceToString(uint128_t value) {
	string valueStr = value.str();
	string result = string(20 - valueStr.length(), '0').append(valueStr);
	result.insert(1, ".");
	result.erase(result.find_last_not_of("0") + 1, result.length());
	return result;
}

// gets pointer to planet with given name
Planet *getPlanetByName(string name) {
	for (auto &item : planets) {
		if (item.name == name)
			return &item;
	}
	return NULL;
}

vector<Planet*> interstellarAlgorithm(Planet *startPlanet, Planet*destinationPlanet) {
	vector<Planet*> priorityQueue,			// priorityQueue holds all nodes that should be computed
					shortestTour;			// storage for shortest tour
	Planet *currentPlanet;					// currentPlanet is always the first element of priorityQueue
	bool firstRun = true;					// flag needed to start algorithm

	startPlanet->distanceToStart = 0;		// init start planet
	priorityQueue.push_back(startPlanet);	// add start planet to priorityQueue

	while (priorityQueue.size() > 0) {
		// sort by shortest distance to start
		sort(priorityQueue.begin(), priorityQueue.end(), [](auto &left, auto &right) {
			return left->distanceToStart < right->distanceToStart;
		});
		// select node with shortest distance to start
		currentPlanet = priorityQueue.front();

		// loop through all possible routes
		for (auto &route : currentPlanet->routes) {
			// dont go backwards
			if (route.first == currentPlanet->previousPlanet) {
				continue;
			}
			// check if distance to planet is shorter than current distance (initial distance -> INF)
			if (route.first->distanceToStart > (currentPlanet->distanceToStart + route.second) || firstRun) {
				// set new distance and previous planet
				route.first->distanceToStart = currentPlanet->distanceToStart + route.second;
				route.first->previousPlanet = currentPlanet;
				
				// remove previous queue entries of that planet if there were any
				priorityQueue.erase(remove(priorityQueue.begin(), priorityQueue.end(), route.first), priorityQueue.end());

				// add planet to priorityQueue
				priorityQueue.push_back(route.first);

				// check if planet is destination planet
				if (route.first == destinationPlanet) {		// if yes, create vector of traveled planets
					// if we already found a route, but this route is shorter
					shortestTour.clear();
					shortestTour.push_back(destinationPlanet);
					while (true) {
						shortestTour.push_back(shortestTour.back()->previousPlanet);
						if (shortestTour.back() == startPlanet) {
							break;
						}
					}
					// sort by smallest distance to start planet
					sort(shortestTour.begin(), shortestTour.end(), [](auto &left, auto &right) {
						return left->distanceToStart < right->distanceToStart;
					});
#ifdef SHOW_ALGORITHM_DEBUGGING
					cout << "Found shortest possible route " << shortestTour.front()->name << " -> " << shortestTour.back()->name << " Distance: " << distanceToString(route.first->distanceToStart) << endl << endl;
					cout << "\tPosition\t\t\tPlanet distance\t\t\t\tTotal distance traveled" << endl << endl;
					for (auto &node : shortestTour) {
						if(node != startPlanet)
							cout << "\t" << node->name << " \t\t\t" << distanceToString(node->distanceToStart - node->previousPlanet->distanceToStart) << " \t\t\t" << distanceToString(node->distanceToStart) << endl;
						else
							cout << "\t" << node->name << endl;
					}
					cout << endl << endl;
#endif
				}
			}
#ifdef SHOW_ALGORITHM_DEBUGGING
			else {
				if (route.first == destinationPlanet) {		// if we get here, we found an alternative route that is longer than previous found routes
					vector<Planet*> tour;
					tour.push_back(currentPlanet);
					while (true) {
						tour.push_back(tour.back()->previousPlanet);
						if (tour.back() == startPlanet) {
							break;
						}
					}
					// sort by smallest distance to start planet
					sort(tour.begin(), tour.end(), [](auto &left, auto &right) {
						return left->distanceToStart < right->distanceToStart;
					});
					
					cout << "Found alternative route " << tour.front()->name << " -> " << destinationPlanet->name << " Distance: " << distanceToString(currentPlanet->distanceToStart + route.second) << endl << endl;
					cout << "\tPosition\t\t\tPlanet distance\t\t\t\tTotal distance traveled" << endl << endl;
					for (auto &node : tour) {
						if (node != startPlanet)
							cout << "\t" << node->name << " \t\t\t" << distanceToString(node->distanceToStart - node->previousPlanet->distanceToStart) << " \t\t\t" << distanceToString(node->distanceToStart) << endl;
						else
							cout << "\t" << node->name << endl;
					}
					cout << "\t" << destinationPlanet->name << " \t\t\t" << distanceToString((currentPlanet->distanceToStart + route.second) - tour.back()->distanceToStart) << " \t\t\t" << distanceToString(currentPlanet->distanceToStart + route.second) << endl;
					cout << endl << endl;
				}
			}
#endif
			firstRun = false;
		}
		// remove computed planet from priorityQueue
		priorityQueue.erase(priorityQueue.begin());
	}
	return shortestTour;
}