using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace L1
{
    class Student
    {
        public string name { get; set; }
        public int year { get; set; }
        public double grade { get; set; }

        public Student(string nm, int yr, double gr)
        {
            this.name = nm;
            this.year = yr;
            this.grade = gr;
        }

    }
}
