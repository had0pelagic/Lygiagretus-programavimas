using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;

namespace L1
{
    class DataMonitor
    {
        private static readonly object balanceLock = new object();
        private string[] array = new string[24];
        private int size = 0;

        public void addStudent(Student student)
        {
            lock (balanceLock)
            {
                while (size == 24)
                    Monitor.Wait(balanceLock);
                for (int i = 0; i < 24; i++)
                {
                    if (this.array[i] == null)
                    {
                        this.array[i] = student.name + "|" + student.year.ToString() + "|" + student.grade.ToString();
                        size++;
                        Monitor.Pulse(balanceLock);
                        break;
                    }
                }
            }
        }
        public string removeStudent(int indicate)
        {
            lock (balanceLock)
            {
                while (size == 0 && Program.readList.students.Count != 0)
                {
                    Console.WriteLine("---------------Thread sleep... " + indicate);
                    Monitor.Wait(balanceLock);
                    Console.WriteLine("++++++++++++++++++Thread not sleeping " + indicate);
                }
                if (size == 0 && Program.readList.students.Count == 0)
                    Monitor.Pulse(balanceLock);
                for (int i = 0; i < 24; i++)
                {
                    if (array[i] != null)
                    {
                        string temp = array[i];
                        Console.WriteLine("Removed : " + temp + " " + i + " " + Program.readList.students.Count);
                        array[i] = null;
                        size--;
                        Monitor.Pulse(balanceLock);
                        return temp;
                    }
                }
                return "";
            }
        }
    }
}
