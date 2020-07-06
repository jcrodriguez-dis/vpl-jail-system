/**
 * package:		Part of vpl-jail-system
 * copyright:
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#include "cgroup.h"
#include <fstream>
#include <iostream>

Cgroup *Cgroup::singlenton=0;

string readCgroupFile(string filename){
	string data;
	ifstream infile;
	infile.open(filename);
	infile >> data;
	infile.close();
	return data;
}

string readCgroupType(){
	string type;
	cout << "Reading from the file cgroup.type" << endl;
	type = readCgroupFile("cgroup.type");
	cout << type << endl;
	return type;
}

//string test;
//test = readCgroupType();
//cout << test << endl;

string readCgroupProcs(){
	string procs;
	cout << "Reading from the file cgroup.procs" << endl;
	procs = readCgroupFile("cgroup.procs");
	cout << procs << endl;
	return procs;
}

string readCgroupThreads(){
	string threads;
	cout << "Reading from the file cgroup.threads" << endl;
	threads = readCgroupFile("cgroup.threads");
	cout << threads << endl;
	return threads;
}

string readCgroupControllers(){
	string controllers;
	cout << "Reading from the file cgroup.controllers" << endl;
	controllers = readCgroupFile("cgroup.controllers");
	cout << controllers << endl;
	return controllers;
}

string readCgroupEvents(){
	string events;
	cout << "Reading from the file cgroup.events" << endl;
	events = readCgroupFile("cgroup.events");
	cout << events << endl;
	return events;
}

string readCgroupMaxDescendants(){
	string maxDesc;
	cout << "Reading from the file cgroup.max.descendants" << endl;
	maxDesc = readCgroupFile("cgroup.max.descendants");
	cout << maxDesc << endl;
	return maxDesc;
}

string readCgroupMaxDepth(){
	string maxDepth;
	cout << "Reading from the file cgroup.max.depth" << endl;
	maxDepth = readCgroupFile("cgroup.max.depth");
	cout << maxDepth << endl;
	return maxDepth;
}

string readCgroupStat(){
	string stat;
	cout << "Reading from the file cgroup.stat" << endl;
	stat = readCgroupFile("cgroup.stat");
	cout << stat << endl;
	return stat;
}

string readCPUStat(){
	string cpu;
	cout << "Reading from the file cgroup.cpu.stat" << endl;
	cpu = readCgroupFile("cgroup.cpu.stat");
	cout << cpu << endl;
	return cpu;
}

string readCPUWeight(){
	string weight;
	cout << "Reading from the file cgroup.cpu.weight" << endl;
	weight = readCgroupFile("cgroup.cpu.weight");
	cout << weight << endl;
	return weight;
}

string readCPUMax(){
	string cpuMax;
	cout << "Reading from the file cgroup.cpu.max" << endl;
	cpuMax = readCgroupFile("cgroup.cpu.max");
	cout << cpuMax << endl;
	return cpuMax;
}

string readMemoryCurrent(){
	string currentMem;
	cout << "Reading from the file cgroup.memory.current" << endl;
	currentMem = readCgroupFile("cgroup.memory.current");
	cout << currentMem << endl;
	return currentMem;
}

string readMemoryLow(){
	string low;
	cout << "Reading from the file cgroup.memory.low" << endl;
	low = readCgroupFile("cgroup.memory.low");
	cout << low << endl;
	return low;
}

string readMemoryHigh(){
	string high;
	cout << "Reading from the file cgroup.memory.high" << endl;
	high = readCgroupFile("cgroup.memory.high");
	cout << high << endl;
	return high;
}

string readMemoryMax(){
	string maxMem;
	cout << "Reading from the file cgroup.memory.max" << endl;
	maxMem = readCgroupFile("cgroup.memory.max");
	cout << maxMem << endl;
	return maxMem;
}

string readMemorySwapCurrent(){
	string currentSwap;
	cout << "Reading from the file cgroup.swap.current" << endl;
	currentSwap = readCgroupFile("cgroup.swap.current");
	cout << currentSwap << endl;
	return currentSwap;
}

string readMemorySwapMax(){
	string maxSwap;
	cout << "Reading from the file cgroup.swap.max" << endl;
	maxSwap = readCgroupFile("cgroup.swap.max");
	cout << maxSwap << endl;
	return maxSwap;
}

string readIOWeight(){
	string ioWeight;
	cout << "Reading from the file cgroup.io.weight" << endl;
	ioWeight = readCgroupFile("cgroup.io.weight");
	cout << ioWeight << endl;
	return ioWeight;
}

string readIOMax(){
	string ioMax;
	cout << "Reading from the file cgroup.io.max" << endl;
	ioMax = readCgroupFile("cgroup.io.max");
	cout << ioMax << endl;
	return ioMax;
}

string readPIDSMax(){
	string pidsMax;
	cout << "Reading from the file cgroup.pids.max" << endl;
	pidsMax = readCgroupFile("cgroup.pids.max");
	cout << pidsMax << endl;
	return pidsMax;
}

string readPIDSCurrent(){
	string currentPids;
	cout << "Reading from the file cgroup.pids.max" << endl;
	currentPids = readCgroupFile("cgroup.pids.max");
	cout << currentPids << endl;
	return currentPids;
}

void writeCgroupFile(string filename, string input){
	ofstream file;
	file.open(filename);
	file << input;
	file.close();
}

void writeCgroupType(string input){
	cout << "Writing to the file cgroup.type" << endl;
	writeCgroupFile("cgroup.type",input);
	cout << "File has been successfully written";
}
void writeCgroupProcs(string input){
	cout << "Writing to the file cgroup.procs" << endl;
	writeCgroupFile("cgroup.procs",input);
	cout << "File has been successfully written";
}
void writeCgroupThreads(string input){
	cout << "Writing to the file cgroup.threads" << endl;
	writeCgroupFile("cgroup.threads",input);
	cout << "File has been successfully written";
}
void writeCgroupSubtreeControl(string input){
	cout << "Writing to the file cgroup.subtree_control" << endl;
	writeCgroupFile("cgroup.subtree_control",input);
	cout << "File has been successfully written";
}
void writeCgroupMaxDesscendants(string input){
	cout << "Writing to the file cgroup.max.descendants" << endl;
	writeCgroupFile("cgroup.max.descendants",input);
	cout << "File has been successfully written";
}
void writeCgroupMaxDepth(string input){
	cout << "Writing to the file cgroup.max.depth" << endl;
	writeCgroupFile("cgroup.max.depth",input);
	cout << "File has been successfully written";
}
void writeCPUWeight(string input){
	cout << "Writing to the file cgroup.cpu.weight" << endl;
	writeCgroupFile("cgroup.cpu.weight",input);
	cout << "File has been successfully written";
}
void writeCPUMax(string input){
	cout << "Writing to the file cgroup.cpu.max" << endl;
	writeCgroupFile("cgroup.cpu.max",input);
	cout << "File has been successfully written";
}
void writeMemoryLow(string input){
	cout << "Writing to the file cgroup.memory.low" << endl;
	writeCgroupFile("cgroup.memory.low",input);
	cout << "File has been successfully written";
}
void writeMemoryHigh(string input){
	cout << "Writing to the file cgroup.memory.high" << endl;
	writeCgroupFile("cgroup.memory.high",input);
	cout << "File has been successfully written";
}
void writeMemoryMax(string input){
	cout << "Writing to the file cgroup.memory.max" << endl;
	writeCgroupFile("cgroup.memory.max",input);
	cout << "File has been successfully written";
}
void writeMemorySwapMax(string input){
	cout << "Writing to the file cgroup.memory.swap.max" <<  endl;
	writeCgroupFile("cgroup.swap.max",input);
	cout << "File has been successfully written";
}
void writeIOWeight(string input){
	cout << "Writing to the file cgroup.io.weight" << endl;
	writeCgroupFile("cgroup.io.weight",input);
	cout << "File has been successfully written";
}
void writeIOMax(string input){
	cout << "Writing to the file cgroup.io.max" << endl;
	writeCgroupFile("cgroup.io.max",input);
	cout << "File has been successfully written";
}
void writePIDSMax(string filename, string input){
	cout << "Writing to the file cgroup.pids.max" << endl;
	writeCgroupFile("cgroup.pids.max",input);
	cout << "File has been successfully written";
}
