using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using System.IO;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.Security.Cryptography;
namespace L1
{
    class Program
    {
        public static Students readList = MainThreadRead();
        public static DataMonitor dataMonitor = new DataMonitor();
        static void Main(string[] args)
        {
            Students readListCopy = MainThreadRead();
            SortedResultMonitor SortedResultMonitor = new SortedResultMonitor();
            SortedResultMonitor.setSize(readList.students.Count);
            Thread[] workerThread = new Thread[8];
            Thread producerThread = new Thread(() => producerT(0, dataMonitor));

            workerThread[0] = new Thread(() => workerT(0, dataMonitor, SortedResultMonitor));
            workerThread[1] = new Thread(() => workerT(1, dataMonitor, SortedResultMonitor));
            workerThread[2] = new Thread(() => workerT(2, dataMonitor, SortedResultMonitor));
            workerThread[3] = new Thread(() => workerT(3, dataMonitor, SortedResultMonitor));
            workerThread[4] = new Thread(() => workerT(4, dataMonitor, SortedResultMonitor));
            workerThread[5] = new Thread(() => workerT(5, dataMonitor, SortedResultMonitor));
            workerThread[6] = new Thread(() => workerT(6, dataMonitor, SortedResultMonitor));
            workerThread[7] = new Thread(() => workerT(7, dataMonitor, SortedResultMonitor));

            string[] array = new string[24];

            foreach (var w in workerThread)
                w.Start();

            producerThread.Start();

            foreach (var w in workerThread)
                w.Join();

            producerThread.Join();

            Output(SortedResultMonitor.array, readListCopy);
            Console.Read();
        }

        public static void producerT(int indicate, DataMonitor dataMonitor)
        {
            foreach (var student in readList.students.Reverse<Student>())
            {
                dataMonitor.addStudent(student);
                readList.students.Remove(student);
            }
        }
        public static void workerT(int indicate, DataMonitor dataMonitor, SortedResultMonitor srm)
        {
            while (true)
            {
                string student = dataMonitor.removeStudent(indicate);
                string[] split = student.Split('|');
                if (split.Length == 3)
                {
                    string hash = split[2];
                    using (MD5CryptoServiceProvider md5 = new MD5CryptoServiceProvider())
                    {
                        UTF8Encoding utf8 = new UTF8Encoding();
                        byte[] data = md5.ComputeHash(utf8.GetBytes(hash));
                        hash = Convert.ToBase64String(data);
                    }
                    int count = hash.Count(c => char.IsUpper(c));
                    if (count > 8)
                        srm.addStudent(student, hash);
                }
                else break;
            }
        }
        public static Students MainThreadRead()
        {
            Students students = null;

            using (StreamReader sr = new StreamReader("1.json"))
                students = JsonConvert.DeserializeObject<Students>(sr.ReadToEnd());

            return students;
        }
        public static void Output(string[] array, Students readlist)
        {
            using (StreamWriter file = new StreamWriter("rez.txt"))
            {
                file.WriteLine("---------------------------------------INPUT-------------------------------------------------------");
                for (int i = 0; i < readlist.students.Count; i++)
                {
                    file.WriteLine(string.Format("{0,-20} {1,20} {2,30}", readlist.students[i].name, readlist.students[i].grade, readlist.students[i].year));
                }
                file.WriteLine("---------------------------------------OUTPUT------------------------------------------------------");
                for (int i = 0; i < array.Length; i++)
                {
                    if (array[i] == null) break;
                    string[] val = array[i].Split('|');
                    file.WriteLine(string.Format("{0,-20} {1,-10} {2,-20} {3,20}", val[0], val[1], val[2], val[3]));
                }
                file.WriteLine("---------------------------------------------------------------------------------------------------");
            }
        }
    }
}
