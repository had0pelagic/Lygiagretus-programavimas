#include <omp.h>
#include <list>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <typeinfo>

using namespace std;
using json = nlohmann::json;

const int max_json_size = 50;
const int max_buffer_size = max_json_size / 2;
const int threads_num = 9;
const string file = "2.json";
const string out_file = "out.txt";

vector<string> split(string val) {
	istringstream st(val);
	vector<string> strs;
	string s;

	while (getline(st, s, '|')) {
		strs.push_back(s);
	}
	return strs;
}

class Student
{
public:
	string name;
	int year;
	double grade;

	Student(string n, int y, double g) {
		name = n;
		year = y;
		grade = g;
	}
};

class SortedResultMonitor {
public:
	int sizeArray = 0;
	string array[max_json_size];
	int size = 0;
	void addStudent(string student, int sum) {

#pragma omp critical (res)
		{
			array[size].append(student).append("|").append(to_string(sum));
			size++;
			for (int i = 0; i < size - 1; i++) {
				for (int j = i + 1; j > 0; j--) {
					vector<string> fval = split(array[j - 1]);
					vector<string> sval = split(array[j]);
					if (stoi(fval.at(1)) > stoi(sval.at(1))) {
						string temp = array[j - 1];
						array[j - 1] = array[j];
						array[j] = temp;
					}
				}
			}
		}
	}
};

class DataMonitor {
	string array[max_buffer_size];
public:
	int size = 0;
	void addStudent(Student &student) {
#pragma omp critical 
		{
			for (int i = 0; i < max_buffer_size; i++) {
				if (array[i].empty()) {
					size++;
					array[i].append(string(student.name)).append("|").append(to_string(student.year)).append("|").append(to_string(student.grade));
					break;
				}
			}
		}
	}

	string removeStudent() {
		string ret;
#pragma omp critical 
		{
			for (int i = 0; i < max_buffer_size; i++) {
				if (!array[i].empty()) {
					ret = array[i];
					array[i].erase();
					size--;
					break;
				}
			}
		}
		return ret;
	}
};

list<Student> readJson() {
	list<Student> ret;
	ifstream fr(file);
	auto json = json::parse(fr);

	for (auto &element : json) {
		Student st(element["name"], element["year"], element["grade"]);
		ret.push_back(st);
	}
	fr.close();
	return ret;
}

int sum(int val) {
	int ret = 0;

	while (val > 0) {
		int num = val % 10;
		val /= 10;
		ret += num;
	}
	return ret;
}

void producerT(DataMonitor &dataMonitor, list<Student> &readList) {
	while (readList.size() > 0) {
		dataMonitor.addStudent(readList.back());
		readList.pop_back();
	}
}

void workerT(DataMonitor &dataMonitor, list<Student> &readList, SortedResultMonitor &sortedResultMonitor) {
	while (true) {
		string student = dataMonitor.removeStudent();
		if (student != "") {
#pragma omp critical
			{
				vector<string> strs = split(student);
				hash<string> hasher;
				size_t hash = hasher(strs.at(2));
				int int_hash = (int)hash;
				int sum_hash = sum(int_hash);
				if (sum_hash > 40)
					sortedResultMonitor.addStudent(student, sum_hash);
			}
		}
		else if (dataMonitor.size == 0 && readList.size() == 0)
			break;
	}
}
void output(SortedResultMonitor &sortedResultMonitor, list<Student> &readList) {
	ofstream wf(out_file);
	int size = 0;

	wf << string(39, '-') << "INPUT" << string(42, '-') << endl;
	wf << '|' << setw(20) << left << "Name" << setw(20) << left << "Year" << setw(15) << left << "Grade" << setw(28) << right << '|' << endl;
	wf << string(86, '-') << endl;
	for (auto &item : readList) {
		wf << '|' << setw(20) << left << item.name << setw(20) << left << item.year << setw(15) << left << item.grade << setw(28) << right << '|' << endl;
	}
	wf << readList.size() << endl;

	wf << string(40, '-') << "OUTPUT" << string(40, '-') << endl;
	wf << '|' << setw(20) << left << "Name" << setw(20) << left << "Year" << setw(18) << left << "Grade" << setw(13) << right << "Hash sum" << setw(18) << '|' << endl;
	wf << string(86, '-') << endl;
	for (int i = 0; i < max_json_size; i++) {
		string val = sortedResultMonitor.array[i];
		vector<string> stud = split(val);
		if (stud.capacity() > 0) {
			size++;
			wf << '|' << setw(20) << left << stud.at(0).c_str() << setw(22) << left << stud.at(1).c_str() << setw(23) << left << stud.at(2).c_str() << stud.at(3).c_str() << setw(22) << right << '|' << endl;
		}
		else break;
	}
	wf << size << endl;
	wf << string(86, '-');

	wf.close();
}
int main() {
	list<Student> readList = readJson();
	list<Student> readListCopy = readList;
	DataMonitor dataMonitor;
	SortedResultMonitor sortedResultMonitor;

#pragma omp parallel num_threads(threads_num)
	{
		/*if (omp_get_thread_num() == 8) {
			printf("MASTER START \n");
			producerT(dataMonitor, readList);
		}
		else
			workerT(dataMonitor, readList, sortedResultMonitor);*/
#pragma omp master
		{
			printf("MASTER START \n");
			producerT(dataMonitor, readList);
		}
		workerT(dataMonitor, readList, sortedResultMonitor);
		cout << "tHREAD::::::::" << omp_get_thread_num() << endl;
	}
	output(sortedResultMonitor, readListCopy);
	cout << "DONE \n";
	return 0;
}