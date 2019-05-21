// HelloWorld.cpp
//
//    В данном примере продемонстрирована базовая установка и использование OpenCL
//    

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

///
//  Константы
//
//const int ARRAY_SIZE = 4096;
//const int ROWS_COUNT = 2048;
const int ARRAY_SIZE = 4096;
const int ROWS_COUNT = 2048;
#pragma comment(linker, "/STACK:100000000")

///
//  Создание OpenCL контекста на основе доступной платформы,
//  использующей GPU (в приоритете) или CPU
//
cl_context CreateContext()
{
    cl_int errNum;
    cl_uint numPlatforms;
    cl_platform_id firstPlatformId;
    cl_context context = NULL;

    // Выберем OpenCL платформу, на которой будет запущен код.
    // В данном примере выберем первую доступную платформу.
    errNum = clGetPlatformIDs(1, &firstPlatformId, &numPlatforms);
    if (errNum != CL_SUCCESS || numPlatforms <= 0)
    {
        std::cerr << "Failed to find any OpenCL platforms." << std::endl;
        return NULL;
    }
    
    // Создадим OpenCL контекст на заданной платформе. 
    // Попробуем создать основанный на GPU контекст и в случае
    // неудача попробуем создать основанный на CPU контекст
    cl_context_properties contextProperties[] =
    {
        CL_CONTEXT_PLATFORM,
        (cl_context_properties)firstPlatformId,
        0
    };
    context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_GPU,
                                      NULL, NULL, &errNum);
    if (errNum != CL_SUCCESS)
    {
        std::cout << "Could not create GPU context, trying CPU..." << std::endl;
        context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_CPU,
                                          NULL, NULL, &errNum);
        if (errNum != CL_SUCCESS)
        {
            std::cerr << "Failed to create an OpenCL GPU or CPU context." << std::endl;
            return NULL;
        }
    }

    return context;
}

///
//  Создание командной очередь для первого доступного
//  устройства из контекста
//
cl_command_queue CreateCommandQueue(cl_context context, cl_device_id *device)
{
    cl_int errNum;
    cl_device_id *devices;
    cl_command_queue commandQueue = NULL;
    size_t deviceBufferSize = -1;

    // Получить размер буфера устройства
    errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &deviceBufferSize);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Failed call to clGetContextInfo(...,GL_CONTEXT_DEVICES,...)";
        return NULL;
    }

    if (deviceBufferSize <= 0)
    {
        std::cerr << "No devices available.";
        return NULL;
    }

    // Выделить память под буфер устройства
    devices = new cl_device_id[deviceBufferSize / sizeof(cl_device_id)];
    errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, deviceBufferSize, devices, NULL);
    if (errNum != CL_SUCCESS)
    {
        delete [] devices;
        std::cerr << "Failed to get device IDs";
        return NULL;
    }

    // Выбор первого доступного устройства
    commandQueue = clCreateCommandQueue(context, devices[0], 0, NULL);
    if (commandQueue == NULL)
    {
        delete [] devices;
        std::cerr << "Failed to create commandQueue for device 0";
        return NULL;
    }

    *device = devices[0];
    delete [] devices;
    return commandQueue;
}

///
//  Создание OpenCL программы из файла-kernel
//
cl_program CreateProgram(cl_context context, cl_device_id device, const char* fileName)
{
    cl_int errNum;
    cl_program program;

    std::ifstream kernelFile(fileName, std::ios::in);
    if (!kernelFile.is_open())
    {
        std::cerr << "Failed to open file for reading: " << fileName << std::endl;
        return NULL;
    }

    std::ostringstream oss;
    oss << kernelFile.rdbuf();

    std::string srcStdStr = oss.str();
    const char *srcStr = srcStdStr.c_str();
    program = clCreateProgramWithSource(context, 1,
                                        (const char**)&srcStr,
                                        NULL, NULL);
    if (program == NULL)
    {
        std::cerr << "Failed to create CL program from source." << std::endl;
        return NULL;
    }

    errNum = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        char buildLog[16384];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                              sizeof(buildLog), buildLog, NULL);

        std::cerr << "Error in kernel: " << std::endl;
        std::cerr << buildLog;
        clReleaseProgram(program);
        return NULL;
    }

    return program;
}

///
//  Создание объектов-памяти для kernel
//  Kernel принимает 3 аргумента: result (вывод), a (ввод),
//  and b (ввод)
//
bool CreateMemObjects(cl_context context, cl_mem memObjects[2], float *a)
{
    memObjects[0] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                   sizeof(float) * ARRAY_SIZE * ROWS_COUNT, a, NULL);
    memObjects[1] = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                   sizeof(float) * ARRAY_SIZE * ROWS_COUNT, NULL, NULL);

    if (memObjects[0] == NULL || memObjects[1] == NULL)
    {
        std::cerr << "Error creating memory objects." << std::endl;
        return false;
    }

    return true;
}

///
//  Очистка от созданных OpenCL ресурсов
//
void Cleanup(cl_context context, cl_command_queue commandQueue,
             cl_program program, cl_kernel kernel, cl_mem memObjects[3])
{
    for (int i = 0; i < 3; i++)
    {
        if (memObjects[i] != 0)
            clReleaseMemObject(memObjects[i]);
    }
    if (commandQueue != 0)
        clReleaseCommandQueue(commandQueue);

    if (kernel != 0)
        clReleaseKernel(kernel);

    if (program != 0)
        clReleaseProgram(program);

    if (context != 0)
        clReleaseContext(context);

}

///
//	main() для HelloWorld
//
int main(int argc, char** argv)
{
    cl_context context = 0;
    cl_command_queue commandQueue = 0;
    cl_program program = 0;
    cl_device_id device = 0;
    cl_kernel kernel = 0;
    cl_mem memObjects[2] = { 0, 0 };
    cl_int errNum;

    // Создание OpenCL контекста для первой доступной платформы
    context = CreateContext();
    if (context == NULL)
    {
        std::cerr << "Failed to create OpenCL context." << std::endl;
        return 1;
    }

    // Создание очереди команд для первого доступного устройства
    // в заданном контексте
    commandQueue = CreateCommandQueue(context, &device);
    if (commandQueue == NULL)
    {
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }

    // Создание OpenCL программы из файла исходного кода HelloWorld.cl для kernel 
    program = CreateProgram(context, device, "Transpose.cl");
    if (program == NULL)
    {
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }

    // Create OpenCL kernel
    cl_int ciErrNum;
    kernel = clCreateKernel(program, "transpose1", &ciErrNum);
    if (kernel == NULL)
    {
        std::cerr << "Failed to create kernel" << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }

    // Создание объектов памяти, используемых kernel.
    // Сначала создаются объекты памяти, содержащие данные
    // для аргументов kernel
    float a[ARRAY_SIZE * ROWS_COUNT];
    float at[ARRAY_SIZE * ROWS_COUNT];
    for (int i = 0; i < ROWS_COUNT; i++)
    { 
        for (int j = 0; j < ARRAY_SIZE; j++)
        {
            a[i * ARRAY_SIZE + j] = (float)j;
            if (a[i * ARRAY_SIZE + j] < 10) {
                //std::cout << 0;
            }
            //std::cout << a[i * ARRAY_SIZE + j] << " ";
        }
        //std::cout << std::endl;
    }

    // Измеряем время работы алгоритма
    auto t1 = std::chrono::high_resolution_clock::now();

    if (!CreateMemObjects(context, memObjects, a))
    {
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }

    // Задание аргументов kernel (a, b, result).
    errNum = clSetKernelArg(kernel, 0, sizeof(cl_mem), &memObjects[0]);
    errNum |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &memObjects[1]);
    errNum |= clSetKernelArg(kernel, 2, sizeof(int), &ARRAY_SIZE);
    errNum |= clSetKernelArg(kernel, 3, sizeof(int), &ROWS_COUNT);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error setting kernel arguments." << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }

    size_t globalWorkSize[2] = { ARRAY_SIZE, ROWS_COUNT };
    size_t localWorkSize[2] = { 32, 32 };

    // Поставить kernel в очередь на исполнение
    errNum = clEnqueueNDRangeKernel(commandQueue, kernel, 2, NULL,
                                    globalWorkSize, localWorkSize,
                                    0, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error queuing kernel for execution." << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }

    // Считать выходной буфер в основную программу
    errNum = clEnqueueReadBuffer(commandQueue, memObjects[1], CL_TRUE,
                                 0, ARRAY_SIZE * ROWS_COUNT * sizeof(float), at,
                                 0, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Error reading result buffer." << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        return 1;
    }

    auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << "Execution time: "
        << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count()
        << std::endl;

    // Вывод результирующего буфера
    //std::cout << std::endl;
    //for (int i = 0; i < ROWS_COUNT; i++)
    //{
    //    for (int j = 0; j < ARRAY_SIZE; j++) {
    //        if (at[i * ARRAY_SIZE + j] < 10) {
    //            std::cout << 0;
    //        }
    //        std::cout << at[i * ARRAY_SIZE + j] << " ";
    //    }
    //    std::cout << std::endl;
    //}
    std::cout << std::endl;
    std::cout << "Executed program succesfully." << std::endl;
    Cleanup(context, commandQueue, program, kernel, memObjects);

    return 0;
}
