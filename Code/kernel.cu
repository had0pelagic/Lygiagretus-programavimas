#include <cuda.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <device_functions.h>
#include "device_launch_parameters.h"
#include "nlohmann/json.hpp"
#include "cuda_runtime.h"

#ifndef CUDACC_RTC
#define CUDACC_RTC
#endif

using namespace std;
using json = nlohmann::json;

const int THREADS = 3;
const int ARRAY_SIZE = 50;
const string IN_FILE = "1.json";
const string OUT_FILE = "out.txt";

class Student
{
public:
	char name[256];
	int year;
	double grade;
	int hash_sum;
};

void readJson(vector<Student> *students) {
	ifstream fr(IN_FILE);
	auto json = json::parse(fr);
	for (auto &element : json) {
		Student stud;
		string n = element["name"];
		strcpy(stud.name, n.c_str());
		stud.year = element["year"];
		stud.grade = element["grade"];
		stud.hash_sum = 0;
		students->push_back(stud);
	}
	fr.close();
}

__device__ int gpu_sum(size_t s) {
	int sum = 0;
	while (s > 0) {
		int num = s % 10;
		s /= 10;
		sum += num;
	}
	return sum;
}

__device__ void gpu_strcpy(char *dest, const char *src) {
	int i = 0;
	do {
		dest[i] = src[i];
	} while (src[i++] != 0);
}

__global__ void gpu_run(Student* device_students, int* device_array_size, int* device_slice_size, Student* device_results, int* device_result_count) {
	unsigned long start_index = *device_slice_size * threadIdx.x;
	unsigned long end_index;

	if (threadIdx.x == blockDim.x - 1)
		end_index = *device_array_size;
	else
		end_index = *device_slice_size * (threadIdx.x + 1);

	printf("///// THREAD: %d -- FROM: %d -- TO: %d \n", threadIdx.x, start_index, end_index);
	for (int i = start_index; i < end_index; i++) {
		Student stud;
		size_t bitshift = 2;
		size_t hash_name = ((size_t)device_students[i].name) >> bitshift;
		size_t hash_year = ((size_t)device_students[i].year) >> bitshift;
		size_t hash_grade = ((size_t)device_students[i].grade) >> bitshift;

		int sum_name = gpu_sum(hash_name);
		int sum_year = gpu_sum(hash_year);
		int sum_grade = gpu_sum(hash_grade);

		int all_sum = sum_grade + sum_year + sum_grade;
		if ((all_sum) % 2 == 0) {
			gpu_strcpy(stud.name, device_students[i].name);
			stud.hash_sum = all_sum;
			stud.year = device_students[i].year;
			stud.grade = device_students[i].grade;
			int index = atomicAdd(device_result_count, 1);
			device_results[index] = stud;
		}
	}
}

void output(vector<Student> student_vec, Student* student, int res_size) {
	ofstream wf(OUT_FILE);

	wf << string(39, '-') << "INPUT" << string(42, '-') << endl;
	wf << '|' << setw(20) << left << "Name" << setw(20) << left << "Year" << setw(15) << left << "Grade" << setw(28) << right << '|' << endl;
	wf << string(86, '-') << endl;
	for (size_t i = 0; i < ARRAY_SIZE; i++)
		wf << '|' << setw(20) << left << student_vec.at(i).name << setw(20) << left << student_vec.at(i).year << setw(15) << left << student_vec.at(i).grade << setw(28) << right << '|' << endl;

	wf << ARRAY_SIZE << endl;

	wf << string(40, '-') << "OUTPUT" << string(40, '-') << endl;
	wf << '|' << setw(20) << left << "Name" << setw(20) << left << "Year" << setw(18) << left << "Grade" << setw(13) << right << "Hash sum" << setw(18) << '|' << endl;
	wf << string(86, '-') << endl;

	for (size_t i = 0; i < res_size; i++)
		wf << '|' << setw(20) << left << student[i].name << setw(22) << left << student[i].year << setw(23) << left << student[i].grade << student[i].hash_sum << setw(22) << right << '|' << endl;
	wf << res_size << endl;
	wf << string(86, '-');

	wf.close();
}

int main() {
	vector<Student> student_vec;
	readJson(&student_vec);

	Student* students = &student_vec[0];
	Student results[ARRAY_SIZE];
	int slice_size = ARRAY_SIZE / THREADS;
	int result_count = 0;

	Student* device_students;
	Student* device_results;
	int* device_array_size;
	int* device_slice_size;
	int* device_result_count;

	cudaMalloc((void**)&device_students, ARRAY_SIZE * sizeof(Student));
	cudaMalloc((void**)&device_array_size, sizeof(int));
	cudaMalloc((void**)&device_slice_size, sizeof(int));
	cudaMalloc((void**)&device_result_count, sizeof(int));

	cudaMalloc((void**)&device_results, ARRAY_SIZE * sizeof(Student));

	cudaMemcpy(device_students, students, ARRAY_SIZE * sizeof(Student), cudaMemcpyHostToDevice);
	cudaMemcpy(device_array_size, &ARRAY_SIZE, sizeof(int), cudaMemcpyHostToDevice);
	cudaMemcpy(device_slice_size, &slice_size, sizeof(int), cudaMemcpyHostToDevice);
	cudaMemcpy(device_result_count, &result_count, sizeof(int), cudaMemcpyHostToDevice);

	gpu_run << <1, THREADS >> > (device_students, device_array_size, device_slice_size, device_results, device_result_count);
	cudaDeviceSynchronize();

	cudaMemcpy(&results, device_results, ARRAY_SIZE * sizeof(Student), cudaMemcpyDeviceToHost);
	int res_size = 0;
	cudaMemcpy(&res_size, device_result_count, sizeof(int), cudaMemcpyDeviceToHost);

	output(student_vec, results, res_size);

	return 0;
}