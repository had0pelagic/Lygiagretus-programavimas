#include <list>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <typeinfo>
#include <mpi.h>

using namespace std;
using namespace MPI;
using json = nlohmann::json;

const int max_json_size = 50;
const int max_buffer_size = max_json_size / 2;
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

void resultThread(int ranks) {
	string array[max_json_size];
	int size = 0;
	int ends = 0;
	while (true) {
		Status status;
		COMM_WORLD.Probe(MPI::ANY_SOURCE, 5, status);//workerThread->resultThread(receive student or end message)
		int l = status.Get_count(MPI::CHAR);
		char *st = new char[l];
		COMM_WORLD.Recv(st, l, MPI::CHAR, status.Get_source(), 5, status);
		string stud(st, l);
		delete[] st;
		cout << "resultThread: " << stud << " rank: " << status.Get_source() << " array size: " << size << " count: " << l << endl;
		if (stud == "END") {
			ends++;
		}
		else {
			array[size].append(stud);
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
		if (ends == ranks) {
			COMM_WORLD.Send(&size, 1, MPI::INT, 0, 8);
			for (int i = 0; i < size; i++) {
				COMM_WORLD.Send(array[i].c_str(), array[i].length(), MPI::CHAR, 0, 6);
			}
			break;
		}
	}
}

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

void workerThread(int rank) {
	while (true) {
		int d = 0;
		COMM_WORLD.Send(&d, 1, MPI::INT, 1, 3);//workerThread(request)->dataThread

		Status status;
		COMM_WORLD.Probe(1, 4, status);//dataThread(receive) student->workerThread
		int l = status.Get_count(MPI::CHAR);
		char *st = new char[l];
		COMM_WORLD.Recv(st, l, MPI::CHAR, 1, 4, status);//dataThread(receive) student->workerThread
		string stud(st, l);
		delete[] st;

		if (stud == "END") {
			cout << "+WORKER DEAD+" << rank << endl;
			string res = "END";
			COMM_WORLD.Send(res.c_str(), res.length(), MPI::CHAR, 2, 5);//workerThread -> resultThread (send message when worker dies)
			break;
		}

		vector<string> strs = split(stud);
		hash<string> hasher;
		size_t hash = hasher(strs.at(2));
		int int_hash = (int)hash;
		int sum_hash = sum(int_hash);
		if (sum_hash > 40) {
			string ret = strs.at(0) + "|" + strs.at(1) + "|" + strs.at(2) + "|" + to_string(sum_hash);
			cout << "workerThread SENDING: " << ret << " " << sum_hash << " WORKER RANK: " << rank << " RET SIZE " << ret.length() << endl;
			COMM_WORLD.Send(ret.c_str(), ret.length(), MPI::CHAR, 2, 5);//workerThread -> resultThread (send student)
		}
	}
}
void output(list<Student> &readListOrg) {
	list<string> readListOut;
	int size = 0;
	Status status;
	COMM_WORLD.Probe(2, 8, status);//resultThread->master(receive size)
	COMM_WORLD.Recv(&size, 1, MPI::INT, 2, 8, status);

	for (int i = 0; i < size; i++) {
		COMM_WORLD.Probe(2, 6, status);//resultThread->master(receive student)
		int l = status.Get_count(MPI::CHAR);
		char *st = new char[l];
		COMM_WORLD.Recv(st, l, MPI::CHAR, 2, 6, status);
		string stud(st, l);
		delete[] st;
		readListOut.push_back(stud);
	}

	ofstream wf(out_file);
	wf << string(39, '-') << "INPUT" << string(42, '-') << endl;
	wf << '|' << setw(20) << left << "Name" << setw(20) << left << "Year" << setw(15) << left << "Grade" << setw(28) << right << '|' << endl;
	wf << string(86, '-') << endl;
	for (auto &item : readListOrg) {
		wf << '|' << setw(20) << left << item.name << setw(20) << left << item.year << setw(15) << left << item.grade << setw(28) << right << '|' << endl;
	}
	wf << readListOrg.size() << endl;

	wf << string(39, '-') << "OUTPUT" << string(42, '-') << endl;
	wf << '|' << setw(20) << left << "Name" << setw(20) << left << "Year" << setw(18) << left << "Grade" << setw(13) << right << "Hash sum" << setw(18) << '|' << endl;
	wf << string(86, '-') << endl;
	for (auto &item : readListOut) {
		vector<string> strs = split(item);
		wf << '|' << setw(20) << left << strs.at(0).c_str() << setw(22) << left << strs.at(1).c_str() << setw(23) << left << strs.at(2).c_str() << strs.at(3).c_str() << setw(22) << right << '|' << endl;
	}
	wf << readListOut.size() << endl;
	wf << string(86, '-');
	wf.close();
}

void masterFill(list<Student> &readList, int size) {
	while (readList.size() > 0) {
		string stud = readList.back().name + "|" + to_string(readList.back().year) + "|" + to_string(readList.back().grade);
		COMM_WORLD.Send(stud.c_str(), stud.length(), MPI::CHAR, 1, 2);
		readList.pop_back();
	}
}

void dataThread(int proc_size) {
	string array[max_buffer_size];
	int size = 0;
	int calc = 0;
	while (true) {
		Status status;
		COMM_WORLD.Probe(0, 2, status);//master->dataThread(master sends student)
		if (size != max_buffer_size) {
			int l = status.Get_count(MPI::CHAR);
			char *st = new char[l];
			COMM_WORLD.Recv(st, l, MPI::CHAR, 0, 2, status);
			string stud(st, l);
			delete[] st;
			for (int i = 0; i < max_buffer_size; i++) {
				if (array[i].empty()) {
					size++;
					array[i].append(stud);
					cout << "dataThread PLACE: " << stud << " SIZE: " << size << endl;
					calc++;
					break;
				}
			}
		}
		else {
			cout << "ARRAY FULL!" << endl;
		}

		COMM_WORLD.Probe(MPI::ANY_SOURCE, 3, status);
		int d = 0;
		COMM_WORLD.Recv(&d, 1, MPI::INT, MPI::ANY_SOURCE, 3, status);
		cout << "dataThread REQUEST FROM: " << status.Get_source() << " d: " << d << endl;
		if (size != 0) {
			string ret = "";
			for (int i = 0; i < max_buffer_size; i++) {
				if (!array[i].empty()) {
					ret = array[i];
					array[i].erase();
					size--;
					break;
				}
			}
			cout << "dataThread SEND: " << ret << " SIZE: " << size << endl;
			COMM_WORLD.Send(ret.c_str(), ret.length(), MPI::CHAR, status.Get_source(), 4);//dataThread->workerThread(after worker request send student to worker)
		}
		else {
			cout << "ARRAY EMPTY!" << endl;
		}

		if (calc == max_json_size && size == 0) {
			for (int i = 3; i < proc_size; i++) {
				string res = "END";
				COMM_WORLD.Send(res.c_str(), res.length(), MPI::CHAR, i, 4);//dataThread->workerThread(send end message)
			}
			break;
		}
	}
}
int main() {
	Init();
	auto rank = COMM_WORLD.Get_rank();
	auto size = COMM_WORLD.Get_size();
	int worker_size = size - 3;

	if (rank > 2) {
		workerThread(rank);
		cout << "WORKER!" << " " << rank << endl;
	}

	if (rank == 0) {
		list<Student> readList = readJson();
		list<Student> readListCopy = readList;
		masterFill(readList, size);
		output(readListCopy);
		cout << "MASTER!" << " " << rank << endl;
	}

	if (rank == 1) {
		dataThread(size);
		cout << "DataThread" << " " << rank << endl;
	}

	if (rank == 2) {
		resultThread(worker_size);
		cout << "ResultThread" << " " << rank << endl;
	}

	Finalize();
	return 0;
}