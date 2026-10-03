// ECE1747 Assignment 1 tutorial - MPI with Threads
// by Soliman Ali 
// Oct 4 2024

#include <iostream>
#include <vector>
#include <cstdlib>
#include <thread>
#include <mutex>
#include <chrono>
#include <iostream>
#include <fstream>  

#include <sstream>  
#include <vector>
#include <string>
#include <unordered_map>
#include <numeric>

#include <queue>
#include <condition_variable>
#include <functional>

#include <cmath>  // For pow() function to calculate r^2
#include <mpi.h>
// #include "C:\Program Files (x86)\Microsoft SDKs\MPI\Include\mpi.h"
// Define Coulomb's constant (in N·m²/C²)
const double k = 8.99e9;

// Class that represents a simple thread pool
class ThreadPool {
public:
    // // Constructor to creates a thread pool with given
    // number of threads
    ThreadPool(size_t num_threads
               = std::thread::hardware_concurrency())
    {

        // Creating worker threads
        for (size_t i = 0; i < num_threads; ++i) {
            threads_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    // The reason for putting the below code
                    // here is to unlock the queue before
                    // executing the task so that other
                    // threads can perform enqueue tasks
                    {
                        // Locking the queue so that data
                        // can be shared safely
                        std::unique_lock<std::mutex> lock(
                            queue_mutex_);

                        // Waiting until there is a task to
                        // execute or the pool is stopped
                        cv_.wait(lock, [this] {
                            return !tasks_.empty() || stop_;
                        });

                        // exit the thread in case the pool
                        // is stopped and there are no tasks
                        if (stop_ && tasks_.empty()) {
                            return;
                        }

                        // Get the next task from the queue
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }

                    task();
                }
            });
        }
    }

    // Destructor to stop the thread pool
    ~ThreadPool()
    {
        {
            // Lock the queue to update the stop flag safely
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }

        // Notify all threads
        cv_.notify_all();

        // Joining all worker threads to ensure they have
        // completed their tasks
        for (auto& thread : threads_) {
            thread.join();
        }
    }

    // Enqueue task for execution by the thread pool
    void enqueue(std::function<void()> task)
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            tasks_.emplace(move(task));
        }
        cv_.notify_one();
    }

private:
    // Vector to store worker threads
    std::vector<std::thread> threads_;

    // Queue of tasks
    std::queue<std::function<void()> > tasks_;

    // Mutex to synchronize access to shared data
    std::mutex queue_mutex_;

    // Condition variable to signal changes in the state of
    // the tasks queue
    std::condition_variable cv_;

    // Flag to indicate whether the thread pool should stop
    // or not
    bool stop_ = false;
};


// Mutex to protect MPI calls.
// We need this because MPI calls are not inherently thread-safe.
// While MPI handles communication across processes, threads handle parallelism within processes.
// So, we need to protect MPI calls with a mutex to ensure thread safety.
std::mutex mpi_mutex;
std::mutex grid_mtx; 
// const int cell_size = 18000;  
// Define the fixed charge value (charge of a proton or electron in Coulombs)
const double q = 1.60e-19;
const double q_SQ = std::pow(q, 2);
const double kq_SQ=k*q_SQ*1e20;
int cell_size=1000;
std::vector<std::vector<int>> readCSV(const std::string& filename) {
    std::vector<std::vector<int>> data;  // To store the result

    std::ifstream file(filename);  // Open the file

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return data;  // Return empty vector if the file cannot be opened
    }

    std::string line;

    // Read file line by line
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string value;
        std::vector<int> row;  // To store each row of integers

        // Read the first integer
        if (std::getline(ss, value, ',')) {
            try {
                row.push_back(std::stoi(value));  // First integer
            } catch (const std::invalid_argument& e) {
                std::cerr << "Invalid data: " << value << " is not a valid integer" << std::endl;
                continue;
            }
        }

        // Read the second integer
        if (std::getline(ss, value, ',')) {
            try {
                row.push_back(std::stoi(value));  // Second integer
            } catch (const std::invalid_argument& e) {
                std::cerr << "Invalid data: " << value << " is not a valid integer" << std::endl;
                continue;
            }
        }

        // Read the third value (either 'p' or 'e')
        if (std::getline(ss, value)) {  // No delimiter here, read until end of line
            if (value == "p") {
                row.push_back(+1);  // Proton, represented by +1
            } else if (value == "e") {
                row.push_back(-1);  // Electron, represented by -1
            } else {
                std::cerr << "Invalid data: " << value << " is not 'p' or 'e'" << std::endl;
                continue;
            }
        }

        // Add the row to the data
        data.push_back(row);
    }

    file.close();  // Close the file

    return data;  // Return the 2D vector with integers
}

double calculateCoulombsForceSQ(double SQdistance) {
    // Coulomb's Law: F = k * (|q1 * q2|) / r^2
    if (SQdistance == 0) {
        return 0.0;
    }
    
    return kq_SQ / SQdistance;
}

double calculateDistanceSQ(int x1, int y1, int x2, int y2) {
    // Use the Euclidean distance formula
    return std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2);
}
double TwoParticles(const std::vector<int>& vec1, const std::vector<int>& vec2, int SQradius) {
    

    // int result=sqrt(std::pow(vec1[0]-vec2[0], 2)+std::pow(vec1[0]-vec2[0],2));
    double SQdistance=calculateDistanceSQ(vec1[0],vec1[1],vec2[0],vec2[1]);

    if (SQdistance > SQradius){
        return 0;
    }
    int flag;
    if (vec1[2] != vec2[2]) {
        flag=-1;
    }
    else{
        flag=1;
    }

    double result=calculateCoulombsForceSQ(SQdistance)*flag;
    return result;
}
double OneParticleWithAll(const std::vector<int>& partcle,std::vector<std::vector<int>> data,int SQradius) {
    double sum_result=0;
    for (const auto& row : data) {
        sum_result+=TwoParticles(partcle,row,SQradius);
    }
    return sum_result;
}
using Cell = std::vector<std::size_t>;
std::unordered_map<std::string, Cell> grid;

// std::string getCellID(int x, int y) {
//     int cell_x = static_cast<int>(x / cell_size);
//     int cell_y = static_cast<int>(y / cell_size);
//     // std::cout << std::to_string(cell_x) + "_" + std::to_string(cell_y) << std::endl;
//     return std::to_string(cell_x) + "_" + std::to_string(cell_y);
// }
std::string getCellID(int x, int y) {
    int cell_x = x/ cell_size;
    int cell_y = (y / cell_size);
    // std::cout << std::to_string(cell_x) + "_" + std::to_string(cell_y) << std::endl;
    return std::to_string(cell_x) + "_" + std::to_string(cell_y);
}
void assignParticlesToGrid(const std::vector<std::vector<int>>& particles) {
    for (std::size_t i = 0; i < particles.size(); ++i) {
        // Get the x, y coordinates of the particle
        int x = particles[i][0];
        int y = particles[i][1];
        
        // Get the cell ID and store the particle index in the corresponding cell
        // std::lock_guard<std::mutex> lock(grid_mtx);
        std::string cell_id = getCellID(x, y);
        grid[cell_id].push_back(i);
    }
}
void assignParticleToGrid(const std::vector<int>& particle, int i) {

        // Get the x, y coordinates of the particle
    int x = particle[0];
    int y = particle[1];
    std::cout << "here" <<x<<y<< std::endl;
    // Get the cell ID and store the particle index in the corresponding cell
    std::string cell_id = getCellID(x, y);
    
    grid[cell_id].push_back(i);
    
}


void writeResultsToCSV(const std::string& filename, const std::vector<double>& results) {
    std::ofstream file(filename);

    // Check if the file is open
    if (!file.is_open()) {
        std::cerr << "Error: Could not open the file " << filename << std::endl;
        return;
    }

    // Write results to the CSV file, one result per line
    for (const double& result : results) {
        file << result << "\n";  // Write each result on a new line
    }

    // Close the file
    file.close();

    std::cout << "Results successfully written to " << filename << std::endl;
}

std::vector<double> readCSV_output(const std::string& filename) {
    std::vector<double> data;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return data;
    }

    std::string line;
    while (std::getline(file, line)) {
        try {

            double value = std::stod(line);
            if (std::isinf(value)){
                value=1;
            }
            data.push_back(value);
        } catch (const std::exception& e) {
            std::cerr << "Error: Could not convert '" << line << "' to a double." << std::endl;
        }
    }

    file.close();
    return data;
}
double calculateAvgErrorPercent(const std::vector<double>& actual, const std::vector<double>& measured) {
    if (actual.size() != measured.size()) {
        std::cerr << "Error: Data files must have the same number of entries." << std::endl;
        return -1;
    }

    double total_error_percent = 0;
    std::size_t n = actual.size();

    for (std::size_t i = 0; i < n; ++i) {
        if (actual[i] != 0) {  // Avoid division by zero
            double error_percent = (std::abs((actual[i] - measured[i]) / actual[i])) * 100;
            // std::cout << "error rate= " << error_percent<< std::endl;
            total_error_percent += error_percent;
        }
    }

    return total_error_percent / n ;  // Return the average error percentage
}

std::vector<std::vector<int>> extractSubVector(const std::vector<std::vector<int>>& data, int start, int end) {
    // Ensure the range is valid
    if (start < 0 || end > data.size() || start >= end) {
        throw std::out_of_range("Invalid range for extracting subvector");
    }
    
    // Create a new vector and copy the range of elements
    std::vector<std::vector<int>> sub_data(data.begin() + start, data.begin() + end);

    return sub_data;
}
void assignParticlesToGridThread(const std::vector<std::vector<int>>& data, int start, int end) {
    for (int i = start; i < end; ++i) {

        // Lock if you're modifying shared grid data
        std::lock_guard<std::mutex> lock(grid_mtx);
        
        // Assign particle i to its grid position
        assignParticleToGrid(data[i],i);  // Replace with actual grid assignment logic
    }
}
double calculateInteraction(const std::vector<int>& particle,int i,int SQradius,const std::vector<std::vector<int>>& data) {
    // Loop over all particles

        int x = particle[0];
        int y = particle[1];
        // Get the cell ID of the current particle
        // std::string current_cell = getCellID(x, y);
        double sum_result=0;
        // Check particles in the current cell and neighboring cells (9 cells total)
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                // Find the neighboring cell
                // std::string neighbor_cell = getCellID(x + dx * cell_size, y + dy * cell_size);
                std::string neighbor_cell = getCellID((x/cell_size + dx) *cell_size, (y/cell_size + dy) * cell_size);
                // std::cout <<x + dx * cell_size<<", "<< y + dy * cell_size <<"; "<< neighbor_cell << std::endl;

                if (grid.find(neighbor_cell) != grid.end()) {
                    // Check each particle in the neighboring cell
                    for (std::size_t neighbor_index : grid[neighbor_cell]) {
                        if (neighbor_index != i) {
                            const auto& neighbor_particle = data[neighbor_index];
                            double dist_sq = calculateDistanceSQ(x, y, neighbor_particle[0], neighbor_particle[1]);
                            if (dist_sq < SQradius) {
                                // Perform interaction (e.g., calculate forces or energy)
                                    int flag;
                                    if (particle[2] != neighbor_particle[2]) {
                                        flag=-1;
                                    }
                                    else{
                                        flag=1;
                                    }
                                sum_result+=calculateCoulombsForceSQ(dist_sq)*flag;
                                
                                
                            }
                        }
                    }
                }
            }
        }

    
    return sum_result;
}
// std::vector<std::vector<int>> buildMap(const std::vector<std::vector<int>>& data,std::vector<std::size_t>& index){
    

// }
std::vector<double> calculateInteractions(const std::vector<std::vector<int>>& data,int SQradius) {
    // Loop over all particles
    std::vector<double> results(data.size(), 0.0);
    // auto start_thread_time = std::chrono::high_resolution_clock::now();
    for (std::size_t i = 0; i < data.size(); ++i) {
        const auto& particle = data[i];
        int x = particle[0];
        int y = particle[1];
        // Get the cell ID of the current particle
        // std::string current_cell = getCellID(x, y);
        
        // Check particles in the current cell and neighboring cells (9 cells total)
        double sum_result = calculateInteraction(data[i],i, SQradius,data);
        results[i] = sum_result;
        // if (i == 5000) {
        if (i % 10000 == 0) {
            std::cout << "Particle " << i <<  " finish" << std::endl;
            // auto end_thread_time = std::chrono::high_resolution_clock::now();
            // std::chrono::duration<double> duration_thread_time = end_thread_time - start_thread_time;
            // std::cout <<"Particle " << i <<  " finish after " << duration_thread_time.count()<<" seconds" << std::endl;
 
        //     // return results;
        }
        // std::cout << "Force= " << sum_result << std::endl;
    }
    return results;
}
void calculateInteractionsThread(const std::vector<std::vector<int>>& data, int SQradius, 
                                 std::vector<double>& results, int start, int end) {
    auto start_thread_time = std::chrono::high_resolution_clock::now();
    for (int i = start; i < end; ++i) {   
        
        double result = calculateInteraction(data[i],i, SQradius,data);
        
        std::lock_guard<std::mutex> lock(grid_mtx);
        results[i]=result;
        
        // if ((i-start) % 10000 == 0) {
        //     // std::cout << "particle "  << i <<std::endl;
        //     // for (int j:data[i]){
        //     //     std::cout << j<< std::endl;
        //     // }
        //     // std::cout << "Particle " << (i-start) <<  " result:" <<result<< std::endl;
        //     std::cout << "Particle " << (i-start) <<  " finish" << std::endl;
        // }
    } 
    auto end_thread_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration_thread_time = end_thread_time - start_thread_time;
    std::cout << "total time for thread calculation:" << duration_thread_time.count() << std::endl;
 
    
}
void calculateInteractions_Worker(const std::vector<std::vector<int>>& data, int SQradius, 
                                 std::vector<double>& results, int start, int end,int position) {
    // Loop over all particles
    for (int i = start; i < end; ++i) {   
        
        double result = calculateInteraction(data[i],i, SQradius,data);
        
        std::lock_guard<std::mutex> lock(grid_mtx);
        results[i-position]=result;

    } 
    
}
std::vector<double> calculateInteractionsThread_Leader(const std::vector<std::vector<int>>& data, int SQradius,int num_threads,int start, int end) {
    int chunk_size=end-start;
    std::vector<double> results(chunk_size, -1);
    int i=0;

    ThreadPool pool(num_threads);
    // int num_of_sub_chunk=num_threads*num_threads;
    int num_of_sub_chunk=num_threads*num_threads*5;
    int sub_chunk_size = chunk_size / num_of_sub_chunk;
    // Enqueue tasks for each subrange in the thread pool
    for (int t = 0; t < num_of_sub_chunk; ++t) {
        int sub_start = start + t * sub_chunk_size;
        int sub_end = (t == num_of_sub_chunk - 1) ? end : start + (t + 1) * sub_chunk_size;
        

        // Enqueue each sub-task into the thread pool
        pool.enqueue([=, &results, &data] {
            calculateInteractions_Worker(data, SQradius, results, sub_start, sub_end, start);
        });
        // if (t %10 == 0) {
        //     // for (int j:data[i]){
        //     //     std::cout << j<< std::endl;
        //     // }
        //     // std::cout << "Particle " << (i-start) <<  " result:" <<result<< std::endl;
        //     std::cout << "Sub chunck " << t <<  " start" << std::endl;
        // }
    }

    return results;
}


int main(int argc, char* argv[]) {
    // Provide the path to your CSV file
    // std::string filename = "test.csv";  // Ensure this file path is correct
    
    int radius;
    int num_threads;
    int mpi_number;
    int ierr;
    int provided;
    auto start_total = std::chrono::high_resolution_clock::now();
    int size, rank;
    // Ensure the mode argument is provided
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mode (1-3)> [radius] [threads_number] [mpi_number]\n";
        return 1;
    }

    // Parse the mode (should be an integer between 1 and 3)
    int mode = std::atoi(argv[1]);
    if (mode < 1 || mode > 3) {
        if (rank == 0) {  // Only rank 0 prints the error
            std::cerr << "Error: Mode must be an integer between 1 and 3.\n";
        }
        MPI_Finalize();
        return 1;
    }

    // Mode 1: Only radius is required
    if (mode == 1) {
        rank=0;
        //don't need MPI
        if (argc < 3) {
            std::cerr << "Error: Mode 1 requires a radius.\n";
            return 1;
        }
        
        // Parse the radius
        radius = std::atof(argv[2]);
        if (radius <= 0) {

            std::cerr << "Error: Radius must be a positive number.\n";
            return 1;
        }
        
        std::cout << "Process " << rank << " running in Mode 1 with radius: " << radius << std::endl;
        // Perform Mode 1 logic here (parallelized work can be based on rank)
    }
    // Mode 2: Radius and thread number are required
    else if (mode == 2) {
        rank=0;
        if (argc < 4) {

            std::cerr << "Error: Mode 2 requires a radius and thread number.\n";
            return 1;
        }
        // Parse the radius
        radius = std::atof(argv[2]);
        if (radius <= 0) {
                std::cerr << "Error: Radius must be a positive number.\n";
            return 1;
        }
        // Parse the number of threads
        num_threads = std::atoi(argv[3]);
        if (num_threads <= 0) {
                std::cerr << "Error: Number of threads must be greater than 0.\n";
            return 1;
        }
        // Optional: Get the maximum hardware concurrency available on the system
        int max_threads = std::thread::hardware_concurrency();
        if ( num_threads > max_threads) {
            std::cerr << "Warning: The number of threads exceeds available hardware concurrency (" 
                      << max_threads << " threads available).\n";
        }

        std::cout << "Process " << rank << " running in Mode 2 with radius: " << radius
                  << " and threads: " << num_threads << std::endl;

        // Perform Mode 2 logic here (using multithreading within each MPI process)
    }

    // Mode 3: Radius, thread number, and MPI number are required
    else if (mode == 3) {
            ierr = MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
            if (ierr != MPI_SUCCESS) {
                char error_string[MPI_MAX_ERROR_STRING];
                int length;
                MPI_Error_string(ierr, error_string, &length);
                std::cerr << "MPI initialization failed: " << error_string << std::endl;
                MPI_Abort(MPI_COMM_WORLD, ierr);
            }

            if (provided < MPI_THREAD_MULTIPLE) {
                std::cerr << "Error: The MPI implementation does not support MPI_THREAD_MULTIPLE." << std::endl;
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
            }

            
            // Get the number of processes from MPI_Comm_size, store in "size" var
            ierr = MPI_Comm_size(MPI_COMM_WORLD, &size);
            if (ierr != MPI_SUCCESS) {
                char error_string[MPI_MAX_ERROR_STRING];
                int length;
                MPI_Error_string(ierr, error_string, &length);
                std::cerr << "MPI_Comm_size failed: " << error_string << std::endl;
                MPI_Abort(MPI_COMM_WORLD, ierr);
            }

            // Get the rank of the process, store in "rank" var
            ierr = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            if (ierr != MPI_SUCCESS) {
                char error_string[MPI_MAX_ERROR_STRING];
                int length;
                MPI_Error_string(ierr, error_string, &length);
                std::cerr << "MPI_Comm_rank failed: " << error_string << std::endl;
                MPI_Abort(MPI_COMM_WORLD, ierr);
            }
        if (argc < 5) {
            if (rank == 0) {
                std::cerr << "Error: Mode 3 requires a radius, thread number, and MPI number.\n";
            }
            MPI_Finalize();
            return 1;
        }

        // Parse the radius
        radius = std::atof(argv[2]);
        if (radius <= 0) {
            if (rank == 0) {
                std::cerr << "Error: Radius must be a positive number.\n";
            }
            MPI_Finalize();
            return 1;
        }

        // Parse the number of threads
        num_threads = std::atoi(argv[3]);
        if (num_threads <= 0) {
            if (rank == 0) {
                std::cerr << "Error: Number of threads must be greater than 0.\n";
            }
            MPI_Finalize();
            return 1;
        }

        // Parse the MPI number (e.g., total number of MPI processes)
        mpi_number = std::atoi(argv[4]);
        if (mpi_number <= 0) {
            if (rank == 0) {
                std::cerr << "Error: MPI number must be greater than 0.\n";
            }
            MPI_Finalize();
            return 1;
        }
        if (size !=mpi_number) {
            if (rank == 0) {
                std::cerr << "Error: MPI number must be the same with  mpiexec -n N\n";
            }
            MPI_Finalize();
            return 0;
        }
        std::cout << "Process " << rank << " running in Mode 3 with radius: " << radius
                  << ", threads: " << num_threads << ", and MPI number: " << mpi_number << std::endl;

        // Perform Mode 3 logic here (using MPI with multithreading and radius)
    }
    cell_size=radius;
    std::string filename = "particles.csv";
    std::string filename_output = "oracle.csv";
    // std::string filename = "particles_skewed.csv";
    // std::string filename_output = "oracle_skewed.csv";
    auto start_read_csv = std::chrono::high_resolution_clock::now();
    std::vector<std::vector<int>> data= readCSV(filename);
    auto end_read_csv = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration_read_csv = end_read_csv - start_read_csv;
    std::cout << "total time for read csv:" << duration_read_csv.count() << std::endl;

    auto start_serial_assign_grid = std::chrono::high_resolution_clock::now();

    assignParticlesToGrid(data);  
    // for (const auto& pair : grid) {
        
    //     std::cout << "Key: " << pair.first << ", Value size: " << pair.second.size() << std::endl;
    // }
    auto end_serial_assign_grid = std::chrono::high_resolution_clock::now();
   std::chrono::duration<double> duration_serial_assign_grid = end_serial_assign_grid - start_serial_assign_grid;

    std::cout << "total time for assign grid:" << duration_serial_assign_grid.count() << std::endl;

    
    std::string combined_outputname;
    int SQradius=radius*radius;
    if (mode == 1){
            std::string outputfile = "output_mode1_";
            auto start_serial = std::chrono::high_resolution_clock::now();
            
            std::vector<double> results=calculateInteractions(data,SQradius);
            auto end_serial = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration_calculation_serial = end_serial - start_serial;
            std::cout << "mode 1 total calculation time:" << duration_calculation_serial.count() << std::endl;
            combined_outputname = outputfile + std::to_string(radius)+".csv";
            // writeResultsToCSV("output_mode1.txt",results);
            writeResultsToCSV(combined_outputname,results);

    }
    else if (mode == 2)
    {
        std::string outputfile = "output_mode2_";
        std::vector<double> results(data.size(), 0.0);
        
        auto start_thread_cal = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        int chunk_size = data.size() / num_threads;
        std::cout << "chunk_size:" << chunk_size << std::endl;
        for (int t = 0; t < num_threads; ++t) {
            int start = t * chunk_size;
            int end = (t == num_threads - 1) ? data.size() : (t + 1) * chunk_size;
            
            threads.emplace_back(calculateInteractionsThread, std::ref(data),SQradius,std::ref(results), start, end);
        }
        // Join threads
        for (auto& th : threads) {
            th.join();
        }
        auto end_thread_cal = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration_thread_cal = end_thread_cal - start_thread_cal;
        std::cout << "total time for thread_cal:" << duration_thread_cal.count() << std::endl;
        combined_outputname = outputfile + std::to_string(radius)+".csv";
        // writeResultsToCSV("output_mode1.txt",results);
        writeResultsToCSV(combined_outputname,results);

    }


    else if (mode ==3){

        
        std::string outputfile = "output_mode3_";
        auto start_mpi_cal = std::chrono::high_resolution_clock::now();
        auto start_data_partition = std::chrono::high_resolution_clock::now();
        int chunk_size = data.size()/mpi_number;
        int start = rank * chunk_size;
        int end = (rank == mpi_number - 1) ? data.size() : (rank + 1) * chunk_size;
        std::cout << "start=  " << start << " end=" <<end<< std::endl;
        auto end_data_partition = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration_data_partition = end_read_csv - start_read_csv;
        std::cout << "total time for data_partition:" << duration_data_partition.count() << std::endl;
        auto start_thread_time = std::chrono::high_resolution_clock::now();

        std::vector<double> results=calculateInteractionsThread_Leader(data,SQradius,num_threads,start,end);
        auto end_thread_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration_thread_time = end_thread_time - start_thread_time;
        std::cout << "total time for thread calculation:" << duration_thread_time.count() << std::endl;
        auto start_gather_result = std::chrono::high_resolution_clock::now();

        int result_size = results.size();
        std::cout << "size"<<size<< std::endl;

        // for (const double& value : results) {
        //     std::cout << value << " ";
        // }
        // Step 1: Gather sizes from each process to the root
        std::vector<int> all_sizes;
        if (rank == 0) { // rank 0 is the root process.
            all_sizes.resize(size);
            std::cout << "Root process " << rank << " is preparing to gather process sums." << std::endl;
        }
        ierr=MPI_Gather(&result_size, 1, MPI_INT, (rank == 0) ?all_sizes.data(): nullptr, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (ierr != MPI_SUCCESS) {
            char error_string[MPI_MAX_ERROR_STRING];
            int length;
            MPI_Error_string(ierr, error_string, &length);
            std::cerr << "MPI_Gather failed on process " << rank << ": " << error_string << std::endl;
            MPI_Abort(MPI_COMM_WORLD, ierr);
        }
        
        // Step 2: Prepare a flattened buffer for gathered results on the root
        std::vector<double> all_flattened_results;
        std::vector<int> displs(size, 0);

        if (rank == 0) {
            // Calculate total size for the flattened result and displacements
            int total_size = std::accumulate(all_sizes.begin(), all_sizes.end(), 0);
            all_flattened_results.resize(total_size);
            // Calculate displacements based on sizes

            for (int i = 1; i < size; ++i) {
                displs[i] = displs[i - 1] + all_sizes[i - 1];
                std::cout <<  displs[i]<<std::endl;
            }

            std::cout << "Root process is gathering all results into a 1D array." << std::endl;
        }

        // Step 3: Gather all data into the flattened 1D array on the root
        MPI_Gatherv(
            results.data(), result_size, MPI_DOUBLE,                  // Send buffer for each process
            all_flattened_results.data(), all_sizes.data(), displs.data(), MPI_DOUBLE, // Receive buffer on root
            0, MPI_COMM_WORLD);
        
        auto end_gather_result = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration_gather_result = end_gather_result - start_gather_result;
        std::cout << "total time for gather results:" << duration_gather_result.count() << std::endl;
        if (rank == 0) {
            std::cout << "Root process " << rank << " received gathered results in rank order:" << std::endl;


            auto end_mpi_cal = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration_mpi_cal = end_mpi_cal - start_mpi_cal;
            std::cout << "total time for mpi_cal:" << duration_mpi_cal.count() << std::endl;
            combined_outputname = outputfile + std::to_string(radius)+".csv";
            // writeResultsToCSV("output_mode1.txt",results);
            writeResultsToCSV(combined_outputname,all_flattened_results);

        }
        else{
            ierr = MPI_Finalize();
            if (ierr != MPI_SUCCESS) {
                char error_string[MPI_MAX_ERROR_STRING];
                int length;
                MPI_Error_string(ierr, error_string, &length);
                std::cerr << "MPI_Finalize failed on process " << rank << ": " << error_string << std::endl;
                return EXIT_FAILURE;
            }
            return 0;
        }
        
    }

    std::vector<double> measured=readCSV_output(combined_outputname);
    std::vector<double> actual=readCSV_output(filename_output);
    double error=calculateAvgErrorPercent(actual,measured);
    std::cout << "avg error rate= " << error<< std::endl;
    auto end_total = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration_total = end_total - start_total;
    std::cout << "total time for the process:" << duration_total.count() << std::endl;

    // Finalize the MPI environment
    if (mode==3){
        std::cout << "Process " << rank << " is finalizing MPI." << std::endl;
        ierr = MPI_Finalize();
        if (ierr != MPI_SUCCESS) {
            char error_string[MPI_MAX_ERROR_STRING];
            int length;
            MPI_Error_string(ierr, error_string, &length);
            std::cerr << "MPI_Finalize failed on process " << rank << ": " << error_string << std::endl;
            return EXIT_FAILURE;
        }
        std::cout << "Process " << rank << " has finalized MPI and is exiting." << std::endl;

    }
    
    return 0;
}





