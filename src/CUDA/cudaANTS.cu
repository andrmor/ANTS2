
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

//=============== declarations  ===============

extern "C" bool cuda_run(const int Method,  //0 - Axial 2D; 1 - XY
                         const int blockSizeXY,
                         const int iterations,
                         const float scale,
                         const float scaleReductionFactor,
                         const int mlORchi2,
                         const bool ignoreLowSigPMs,
                         const float ignoreThresholdLow,
                         const float ignoreThresholdHigh,
                         const bool ignoreFarPMs,
                         const float ignoreDistance,
                         const float comp_r0,
                         const float comp_a,
                         const float comp_b,
                         const float comp_lam2,
                         const float* const PMx,
                         const float* const PMy,
                         const int numPMs,
                         const float* lrfdata,
                         const int lrfFloatsPerPM,
                         const int p1,
                         const int p2,
                         const int lrfFloatsAxialPerPM,  //only for Composite!
                         const float* EventsData,
                         const int numEvents,
                         float *RecX,
                         float *RecY,
                         float *RecEnergy,
                         float *Chi2,
                         float *Probability,
                         float *ElapsedTime);

__global__ void kernelRadial2D(const bool mlORchi2,
                               const bool ignoreLowSignalPMs,
                               const float ignoreThresholdLow,
                               const float ignoreThresholdHigh,
                               const bool ignoreFarPMs,
                               const float ignoreDistance2,
                               const int iterations,
                               float scale,
                               const float scaleReductionFactor,               
                               const float* pmx,
                               const float* pmy,
                               int numPMs,
                               int lrfSizePerPM,
                               const float * const d_lrfData,
                               const float* const d_eventsData,
                               int numEvents,
                               float* d_recX,
                               float* d_recY,
                               float* d_recEnergy,
                               float* d_chi2);

__global__ void kernelRadial2Dcomp(const bool mlORchi2,
                               const bool ignoreLowSignalPMs,
                               const float ignoreThresholdLow,
                               const float ignoreThresholdHigh,
                               const bool ignoreFarPMs,
                               const float ignoreDistance2,
                               const int iterations,
                               float scale,
                               const float scaleReductionFactor,
                               const float comp_r0,
                               const float comp_a,
                               const float comp_b,
                               const float comp_lam2,
                               const float* pmx,
                               const float* pmy,
                               int numPMs,
                               int lrfSizePerPM,
                               const float * const d_lrfData,
                               const float* const d_eventsData,
                               int numEvents,
                               float* d_recX,
                               float* d_recY,
                               float* d_recEnergy,
                               float* d_chi2);

__global__ void kernelComposite(const bool mlORchi2,
                                const bool ignoreLowSigPMs,
                                const float ignoreThresholdLow,
                                const float ignoreThresholdHigh,
                                const bool ignoreFarPMs,
                                const float ignoreDistance2,
                                const int iterations,
                                float scale,
                                const float scaleReductionFactor,
                                const float comp_r0,
                                const float comp_a,
                                const float comp_b,
                                const float comp_lam2,
                                const bool fCompressed,
                                const float* pmx,
                                const float* pmy,
                                int numPMs,
                                int lrfFloatsPerPM,
                                int lrfFloatsAxialPerPM,
                                int nintx,
                                int ninty,
                                const float* const d_lrfData,
                                const float* const d_eventsData,
                                int numEvents,
                                float* d_recX,
                                float* d_recY,
                                float* d_recEnergy,
                                float* d_chi2,
                                float* d_probability);

__global__ void kernelXY(const bool mlORchi2,
                               const bool ignoreLowSignalPMs,
                               const float ignoreThresholdLow,
                               const float ignoreThresholdHigh,
                         const bool ignoreFarPMs,
                         const float ignoreDistance2,
                         const float* pmx,
                         const float* pmy,
                               const int iterations,
                               float scale,
                               const float scaleReductionFactor,
                               int numPMs,
                               int lrfSizePerPM,
                               const int nintx,
                               const int ninty,
                               const float * const d_lrfData,
                               const float* const d_eventsData,
                               int numEvents,
                               float* d_recX,
                               float* d_recY,
                               float* d_recEnergy,
                               float* d_chi2,
                               float *d_probability);


//__constant__ float d_LRF[16384];  //constant memory - former was containing LRF data


//=============== implementation ===============

static char lastError[100];

//void checkCUDAError(const char* msg)
bool checkCUDAError(const char* msg)  //true if error found
{
   // Print a message if a CUDA error occurred
   cudaError_t err = cudaGetLastError();

   if (cudaSuccess != err)
     {
      fprintf(stderr,"\n-->CUDA ERROR: %s: %s.\n", msg, cudaGetErrorString(err));      
      sprintf(lastError, "CUDA ERROR: %s: %s.\n", msg, cudaGetErrorString(err));
      //exit(EXIT_FAILURE);
      return true;
     }
   else
     {
       sprintf(lastError, "%s", "");
     }
   return false;
}

extern "C" const char *getLastCUDAerror()
{
  return lastError;
}

extern "C" bool cuda_run(const int Method,  //0 - Axial 2D; 1 - XY; 3 - CompRad
                         const int blockSizeXY,
                         const int iterations,
                         const float scale,
                         const float scaleReductionFactor,
                         const int mlORchi2,
                         const bool ignoreLowSigPMs,
                         const float ignoreThresholdLow,
                         const float ignoreThresholdHigh,
                         const bool ignoreFarPMs,
                         const float ignoreDistance,
                         const float comp_r0,
                         const float comp_a,
                         const float comp_b,
                         const float comp_lam2,
                         const float* const PMx,
                         const float* const PMy,
                         const int numPMs,
                         const float* lrfdata,
                         const int lrfFloatsPerPM,
                         const int p1,
                         const int p2,
                         const int lrfFloatsAxialPerPM,
                         const float* EventsData,
                         const int numEvents,
                         float *RecX,
                         float *RecY,
                         float *RecEnergy,
                         float *Chi2,
                         float *Probability,
                         float *ElapsedTime)
{  
  //Allocate global memory on GPU for input/output event data
  int sizeEventsBuffer  = numEvents * (numPMs+2) * sizeof(float); //active PMs + XY of offset
  int sizeEvents = numEvents * sizeof(float);
  int sizePMsFloat = numPMs * sizeof(float);
  int sizeLRFsFloat = numPMs * lrfFloatsPerPM * sizeof(float);

  // Device data
  float* d_eventsData = 0;
  float* d_pmx = 0;
  float* d_pmy = 0;
  float* d_lrfData = 0;

  float* d_recX = 0;
  float* d_recY = 0;
  float* d_recEnergy = 0;
  float* d_chi2 = 0;
  float* d_probability = 0;

  fprintf(stderr,"cuda==> Device memory allocation\n");
    //input
  cudaMalloc((void**) &d_eventsData, sizeEventsBuffer);
  cudaMalloc((void**) &d_lrfData, sizeLRFsFloat);

  if (checkCUDAError("Memory alloc (eventsData bufer)")) return false;
  if (Method == 0 || Method == 3 || Method == 4)
    {
      cudaMalloc((void**) &d_pmx, sizePMsFloat);
      if (checkCUDAError("Memory alloc (pmx)")) return false;
      cudaMalloc((void**) &d_pmy, sizePMsFloat);
      if (checkCUDAError("Memory alloc (pmy)")) return false;
    }

    //output
  cudaMalloc((void**) &d_recX, sizeEvents);
  if (checkCUDAError("Memory alloc (recX)")) return false;
  cudaMalloc((void**) &d_recY, sizeEvents);
  if (checkCUDAError("Memory alloc (recY)")) return false;
  cudaMalloc((void**) &d_recEnergy, sizeEvents);
  if (checkCUDAError("Memory alloc (recEnergy)")) return false;
  cudaMalloc((void**) &d_chi2, sizeEvents);
  if (checkCUDAError("Memory alloc (chi2)")) return false;
  if (mlORchi2 == 0) //ML only
    {
      cudaMalloc((void**) &d_probability, sizeEvents);
      if (checkCUDAError("Memory alloc (probability)")) return false;
    }


  // copy events to GPU global memory
  cudaMemcpy(d_eventsData, EventsData, sizeEventsBuffer, cudaMemcpyHostToDevice);
  if (checkCUDAError("Copy events data to GPU (events data)")) return false;
  if (Method == 0 || Method == 3 || Method == 4)
    {
      cudaMemcpy(d_pmx, PMx, sizePMsFloat, cudaMemcpyHostToDevice);
      if (checkCUDAError("Copy events data to GPU (PMx)")) return false;
      cudaMemcpy(d_pmy, PMy, sizePMsFloat, cudaMemcpyHostToDevice);
      if (checkCUDAError("Copy events data to GPU (PMy)")) return false;
    }

/*
  //setting up constant memory
  fprintf(stderr,"cuda==> LRF floats Per PM: %i )\n",lrfFloatsPerPM);
  int lrfdatasize = numPMs * lrfFloatsPerPM * sizeof(float);
  fprintf(stderr,"cuda==> Allocation of the constant memory (%i bytes)\n", lrfdatasize);
  cudaMemcpyToSymbol(d_LRF, lrfdata, lrfdatasize);
  if (checkCUDAError("Copy LRF data to constant memory")) return false;
*/
  cudaMemcpy(d_lrfData, lrfdata, sizeLRFsFloat, cudaMemcpyHostToDevice);
  if (checkCUDAError("Copy LRF data to GPU")) return false;

  dim3 threads(blockSizeXY,blockSizeXY,1);

  //calculating total ammount of needed shared memory
  int sizeSharedMem = 0;
  if (Method == 0 || Method == 3 || Method == 4) sizeSharedMem += numPMs * 2; //PMx and PMy
  sizeSharedMem += numPMs; //one-event data
  if (mlORchi2 == 0) sizeSharedMem += blockSizeXY*blockSizeXY; //probability - ML only
  sizeSharedMem += blockSizeXY*blockSizeXY; // X
  sizeSharedMem += blockSizeXY*blockSizeXY; //  Y
  sizeSharedMem += blockSizeXY*blockSizeXY; //  Energy
  sizeSharedMem += blockSizeXY*blockSizeXY; //  Chi2
  sizeSharedMem *= 4; // all data on shared use 32 bit words
  fprintf(stderr,"cuda==> Calculated ammount of shared memory usage: %i bytes\n", sizeSharedMem);

  int blocksX, blocksY;
  if (numEvents <  65536)
    {
      blocksX = numEvents;
      blocksY = 1;
    }
  else
    {
      blocksY = numEvents / 65535;
      if (numEvents % 65535 != 0) blocksY++;
      blocksX = numEvents / blocksY;
      if (numEvents % blocksY != 0) blocksX++;
    }
  fprintf(stderr,"cuda==> Invoking kernel with (%i, %i) grid and (%i, %i) threads...\n", blocksX, blocksY, blockSizeXY, blockSizeXY);
  dim3 grid(blocksX, blocksY, 1);

  //time control
  float elapsedTime;
  cudaEvent_t start;
  cudaEvent_t stop;
  cudaEventCreate(&start);
  cudaEventCreate(&stop);
  cudaEventRecord(start, 0);

  //running kernel
  switch (Method)
    {
    case 0: // Axial
      kernelRadial2D <<< grid, threads, sizeSharedMem>>>(mlORchi2,
                                                         ignoreLowSigPMs,
                                                         ignoreThresholdLow,
                                                         ignoreThresholdHigh,
                                                         ignoreFarPMs,
                                                         ignoreDistance*ignoreDistance,
                                                         iterations,
                                                         scale,
                                                         scaleReductionFactor,                                             
                                                         d_pmx,
                                                         d_pmy,
                                                         numPMs,
                                                         lrfFloatsPerPM,
                                                         d_lrfData,
                                                         d_eventsData,
                                                         numEvents,
                                                         d_recX,
                                                         d_recY,
                                                         d_recEnergy,
                                                         d_chi2);

      break;
    case 3: //CompRad
      kernelRadial2Dcomp <<< grid, threads, sizeSharedMem>>>(mlORchi2,
                                                         ignoreLowSigPMs,
                                                         ignoreThresholdLow,
                                                         ignoreThresholdHigh,
                                                         ignoreFarPMs,
                                                         ignoreDistance*ignoreDistance,
                                                         iterations,
                                                         scale,
                                                         scaleReductionFactor,                                                         
                                                         comp_r0,
                                                         comp_a,
                                                         comp_b,
                                                         comp_lam2,
                                                         d_pmx,
                                                         d_pmy,
                                                         numPMs,
                                                         lrfFloatsPerPM,
                                                         d_lrfData,
                                                         d_eventsData,
                                                         numEvents,
                                                         d_recX,
                                                         d_recY,
                                                         d_recEnergy,
                                                         d_chi2);

      break;
    case 1:
      //kernelFreeform <<< grid, threads, sizeSharedMem>>>(mlORchi2,
            kernelXY <<< grid, threads, sizeSharedMem>>>(mlORchi2,
                                                         ignoreLowSigPMs,
                                                         ignoreThresholdLow,
                                                         ignoreThresholdHigh,
                                                       ignoreFarPMs,
                                                       ignoreDistance*ignoreDistance,
                                                       d_pmx,
                                                       d_pmy,
                                                         iterations,
                                                         scale,
                                                         scaleReductionFactor,
                                                         numPMs,
                                                         lrfFloatsPerPM,
                                                         p1, // nintx
                                                         p2, // ninty
                                                         d_lrfData,
                                                         d_eventsData,
                                                         numEvents,
                                                         d_recX,
                                                         d_recY,
                                                         d_recEnergy,
                                                         d_chi2,
                                                         d_probability);

      break;
    case 4:
      kernelComposite <<< grid, threads, sizeSharedMem>>>(mlORchi2,
                                                         ignoreLowSigPMs,
                                                         ignoreThresholdLow,
                                                         ignoreThresholdHigh,
                                                         ignoreFarPMs,
                                                         ignoreDistance*ignoreDistance,
                                                         iterations,
                                                         scale,
                                                         scaleReductionFactor,
                                                         comp_r0,
                                                         comp_a,
                                                         comp_b,
                                                         comp_lam2,
                                                         (comp_r0!=0 || comp_a!=0 || comp_b!=0 || comp_lam2!=0),
                                                         d_pmx,
                                                         d_pmy,
                                                         numPMs,
                                                         lrfFloatsPerPM,
                                                         lrfFloatsAxialPerPM,
                                                         p1, // nintx
                                                         p2, // ninty
                                                         d_lrfData,
                                                         d_eventsData,
                                                         numEvents,
                                                         d_recX,
                                                         d_recY,
                                                         d_recEnergy,
                                                         d_chi2,
                                                         d_probability);
      break;
    default:
      break;
    }

  cudaThreadSynchronize();
  cudaEventRecord(stop, 0);
  cudaEventSynchronize(stop);
  cudaEventElapsedTime(&elapsedTime, start, stop);
  *ElapsedTime = elapsedTime;

  if (checkCUDAError("Kernel invocations")) return false;
  fprintf(stderr,"cuda==> Kernel invocation successful\n");

  fprintf(stderr,"cuda==> kernel execution elapsed time: %f ms\n", elapsedTime);


  // copy results to CPU memory
  cudaMemcpy(RecX, d_recX, sizeEvents, cudaMemcpyDeviceToHost);
  if (checkCUDAError("Copy results to CPU (recX)")) return false;
  cudaMemcpy(RecY, d_recY, sizeEvents, cudaMemcpyDeviceToHost);
  if (checkCUDAError("Copy results to CPU (recY)")) return false;
  cudaMemcpy(RecEnergy, d_recEnergy, sizeEvents, cudaMemcpyDeviceToHost);
  if (checkCUDAError("Copy results to CPU (recEnergy)")) return false;
  cudaMemcpy(Chi2, d_chi2, sizeEvents, cudaMemcpyDeviceToHost);
  if (checkCUDAError("Copy results to CPU (chi2)")) return false;
  if (mlORchi2 == 0) //ML only
    {
      cudaMemcpy(Probability, d_probability, sizeEvents, cudaMemcpyDeviceToHost);
      if (checkCUDAError("Copy results to CPU (probability)")) return false;
    }

  cudaEventDestroy(start);
  cudaEventDestroy(stop);

  cudaFree(d_eventsData);
  if (Method == 0 || Method == 3)
    {
      cudaFree(d_pmx);
      cudaFree(d_pmy);
    }
  cudaFree(d_recX);
  cudaFree(d_recY);
  cudaFree(d_recEnergy);
  cudaFree(d_chi2);
  if (mlORchi2 == 0) cudaFree(d_probability); //ML only
  if (checkCUDAError("Cuda memory free")) return false;

  return true;
}

__global__ void kernelRadial2D(const bool mlORchi2,
                               const bool ignoreLowSignalPMs,
                               const float ignoreThresholdLow,
                               const float ignoreThresholdHigh,
                               const bool ignoreFarPMs,
                               const float ignoreDistance2,
                               const int iterations,
                               float scale,
                               const float scaleReductionFactor,                 
                               const float* pmx,
                               const float* pmy,
                               int numPMs,
                               int lrfSizePerPM, //intervals + 1
                               const float* const d_lrfData,
                               const float* const d_eventsData,
                               int numEvents,
                               float* d_recX,
                               float* d_recY,
                               float* d_recEnergy,
                               float* d_chi2)

{
  //setting up shared memory
  int activeThreads = blockDim.x * blockDim.y;

  extern __shared__ float shared[];  //these data are shared withing a thread block - that is for all points of the grid (one event!)
  float* PMx = (float*) &shared;
  float* PMy = (float*) &PMx[numPMs];
  float* signal = (float*) &PMy[numPMs];
  int* nodeIX = (int*) &signal[numPMs];
  int* nodeIY = (int*) &nodeIX[activeThreads];
  float* nodeChi2 = (float*) &nodeIY[activeThreads];
  float* nodeEnergy = (float*) &nodeChi2[activeThreads];
    //for ML only
  float* nodeProb = (float*) &nodeEnergy[activeThreads];

  __shared__ float Xoffset;
  __shared__ float Yoffset;
  __shared__ float Xoffset0; //will be kept unchanged, for dynamic passives by distance
  __shared__ float Yoffset0; //will be kept unchanged, for dynamic passives by distance

  int ievent = blockIdx.x + gridDim.x * blockIdx.y;      //cuda GRID is used to select an event - below word "grid" refers to the grid of XYs positions
  int threadID = threadIdx.x + threadIdx.y * blockDim.x; //block is used to scan XY (each node - one thread)
                                                         //threadIdx.x is related to offset in X from Xcenter of the grid
                                                         //threadIdx.y is related to offset in Y from Ycenter of the grid

  //to do!!! case when numPMs>numtreads in block
  if (threadID<numPMs)
    { //in this block _only_: threadID is PMs index
      PMx[threadID] = pmx[threadID];
      PMy[threadID] = pmy[threadID];

      signal[threadID] = d_eventsData[ievent*(numPMs+2) + threadID]; //buffer contains PM signals and XY offset
    }

  if (threadID == 0)
    { //0th tread sets the center of the grid
      Xoffset = d_eventsData[ievent*(numPMs+2) + numPMs];  //offsetX;
      Xoffset0 = Xoffset;
      Yoffset = d_eventsData[ievent*(numPMs+2) + numPMs+1];//offsetY;
      Yoffset0 = Yoffset;
    }

  __syncthreads(); //need to synchronize - all shared input data are ready

  int activePMs;

  //starting multigrid iterations
  for (int iter=0; iter<iterations; iter++)
   {
     float X = Xoffset + scale * (-0.5*(blockDim.x - 1)+ threadIdx.x); //coordinates of this node (Thread)
     float Y = Yoffset + scale * (-0.5*(blockDim.y - 1)+ threadIdx.y);

     if (ievent<numEvents)
       {
         activePMs = 0;
         //since we cannot save lrfs in a fast tmp storage, collecting all statistics in parralel
         //energy - SUM sig(i) / SUM LRF(i)
         float sumsig = 0;
         float sumLRF = 0;
         //probability for ML: SUM sig(i)*ln{LRF(i)*E} - LRF(i)*E = SUM   sig(i)*ln(LRF(i)) + sig(i)*ln(E) - LRF(i)*E
         float sumsigLnLRF = 0;
         //for chi2:  SUM   {sig(i) - LRF(i)*E)^2 / LRF(i)E  =
         //           SUM   sig2/lrf/E -2*sig +lrf*E
         float sumsig2overLRF = 0;

         bool isBadNode = false; //can be set to true if range to one of PMs larger than the defininition range of its LRF; or energy < 0
         for (int ipm=0; ipm<numPMs; ipm++)
           {
             float tsig = signal[ipm];
             if (ignoreLowSignalPMs)
                if (tsig < ignoreThresholdLow || tsig > ignoreThresholdHigh)
                 {
                  //signal < threshold, ignoring this PM
                  continue;
                 }
             if (ignoreFarPMs)
               {
                 float stX  = (PMx[ipm] - Xoffset0);
                 float stY  = (PMy[ipm] - Yoffset0);
                 float r2 = stX*stX + stY*stY;// r  = sqrtf(r);
                 //distance2 from this PM center to the start position of the search
                 if (r2 > ignoreDistance2)
                  {
                   //too far from this PM center, ignoring this PM
                   continue;
                  }
               }


             float dx = d_lrfData[lrfSizePerPM*ipm + lrfSizePerPM-1]; //LRFs are defined in the range from 0 to dx
             float offX  = (X - PMx[ipm]);
             float offY  = (Y - PMy[ipm]);
             float r = offX*offX + offY*offY;
             r  = sqrtf(r); //distance to the PM center
             if (r > dx)
               {
                 //outside! cannot use this thread (node) at all
                 isBadNode = true;
                 break;
               }

             activePMs++; //this one is active
             sumsig += tsig;             

             //LRF calculation
             if (r == dx) dx -= 1.0e-7;
             int nint = lrfSizePerPM-4; // lrfSizePerPM = number of intervals +3 + 1
             float xi = r/dx*nint;
             int ix = (int)xi;
             float xf = xi-ix;

             float a0, a1, a2;
             float p0, p1, p2, p3;

             a0 = d_lrfData[lrfSizePerPM*ipm + ix];
             a1 = d_lrfData[lrfSizePerPM*ipm + ix + 1];
             float c2 = d_lrfData[lrfSizePerPM*ipm + ix + 2];
             a2 = a0 + a1 + c2;  // c0 + c1 + c2
             a1 = a1 + a1 + a1;  // 3*c1
             p0 = a2 + a1;       // c0 + 4*c1 + c2
             p2 = a2 - a1;       // c0 - 2*c1 + c2
             p2 = p2 + p2 + p2;  // 3*c0 - 6*c1 + 3*c2
             p1 = c2 - a0;       // c2 - c0
             p1 = p1 + p1 + p1;  // 3*c2 - 3*c0
             p3 = a0 + a0 - a1 - p2 + d_lrfData[lrfSizePerPM*ipm + ix + 3]; // -c0 + 3*c1 - 3*c2 + c3

             float lrf = (p0 + xf*(p1 + xf*(p2 + xf*p3)))/6.;

             //have lrf now
             if (lrf <= 0)
               { //bad lrf
                 isBadNode = true;
                 break;
               }
             sumLRF += lrf;
             if (mlORchi2 == 0) sumsigLnLRF += tsig * __logf(lrf);
             sumsig2overLRF += tsig * tsig / lrf;

           } //end cycle by PMs

         //can calculate energy ("naive" approach)
         float energy = 1.0;
         if (sumLRF > 0 ) energy = sumsig/sumLRF;
         if (energy<1.0e-10) isBadNode = true;

         //can calculate probability and chi2
         //storing results in shared memory
         if (isBadNode)
           {
             if (mlORchi2 == 0) nodeProb[threadID] = -1.0e10;
             nodeChi2[threadID] = 1.0e10;
           }
         else
           {
             if (mlORchi2 == 0) nodeProb[threadID] = sumsigLnLRF  +  sumsig * __logf(energy)  -  sumLRF*energy;
             nodeChi2[threadID] = sumsig2overLRF/energy -2*sumsig + sumLRF*energy;
           }

         nodeIX[threadID] = threadIdx.x;
         nodeIY[threadID] = threadIdx.y;
         nodeEnergy[threadID] = energy;
       }
     else
       {
         //could happen for num events >65535
         //wrong event index - we dont care, these data are not sent to host
         //only important: these threads still can see all __syncthreads()
       }

     __syncthreads(); //to prepare node data

     //looking for the best location - compare nodes always in pairs - using shared memory
      int Nodes = activeThreads;    //activeThreads = blockDim.x * blockDim.y
      do
              {
                int idx = Nodes / 2;
                //---

                if (mlORchi2 == 0) //ML
                  {
                    if (threadID < idx && nodeProb[threadID] < nodeProb[threadID + idx])
                      {
                       nodeProb[threadID] = nodeProb[threadID + idx];
                       nodeIX[threadID] = nodeIX[threadID + idx];
                       nodeIY[threadID] = nodeIY[threadID + idx];
                       nodeEnergy[threadID] = nodeEnergy[threadID + idx];
                       nodeChi2[threadID] = nodeChi2[threadID + idx];
                      }

                    if (threadID == 0)
                     if (Nodes % 2 != 0)  //for the cases when Nodes is odd number
                      {
                        if (nodeProb[0] < nodeProb[Nodes-1]) //all other nodes are already covered
                          {
                           nodeProb[0] = nodeProb[Nodes-1];
                           nodeIX[0] = nodeIX[Nodes-1];
                           nodeIY[0] = nodeIY[Nodes-1];
                           nodeEnergy[0] = nodeEnergy[Nodes-1];
                           nodeChi2[0] = nodeChi2[Nodes-1];
                          }
                      }
                  }
                else
                  {
                    if (threadID < idx && nodeChi2[threadID] > nodeChi2[threadID + idx])
                      {
                       nodeIX[threadID] = nodeIX[threadID + idx];
                       nodeIY[threadID] = nodeIY[threadID + idx];
                       nodeEnergy[threadID] = nodeEnergy[threadID + idx];
                       nodeChi2[threadID] = nodeChi2[threadID + idx];
                      }

                    if (threadID == 0)
                     if (Nodes % 2 != 0)  //for the cases when Nodes is odd number
                      {
                        if (nodeChi2[0] > nodeChi2[Nodes-1]) //all other nodes are already covered
                          {
                           nodeIX[0] = nodeIX[Nodes-1];
                           nodeIY[0] = nodeIY[Nodes-1];
                           nodeEnergy[0] = nodeEnergy[Nodes-1];
                           nodeChi2[0] = nodeChi2[Nodes-1];
                          }
                      }
                  }


                //---
                Nodes = idx;
                __syncthreads();
              }
            while (Nodes>1);

     // thread index 0 contains info on to the best fit location with the current grid
     if (threadID == 0)
       {
         Xoffset +=  scale * (-0.5*(blockDim.x - 1)+ nodeIX[0]); //new gid center - for the next iteration
         Yoffset +=  scale * (-0.5*(blockDim.y - 1)+ nodeIY[0]);
       }
     __syncthreads();


     //preparing scale for the next iteration
     scale /= scaleReductionFactor;
   }//iteration end

  //reporting back reconstructed event data
  if (threadID == 0 && ievent<numEvents)
      {
        //reporting results
        d_recX[ievent] = Xoffset;
        d_recY[ievent] = Yoffset;
        d_recEnergy[ievent] = nodeEnergy[0];
        int df = activePMs - 4;  //-1 -2XY -1energy
        if (df < 1) df = 1;
        d_chi2[ievent] = nodeChi2[0] / df;
      }
}

//it is identical to the kernelRadial2 except handling radial compression
__global__ void kernelRadial2Dcomp(const bool mlORchi2,
                               const bool ignoreLowSignalPMs,
                               const float ignoreThresholdLow,
                               const float ignoreThresholdHigh,
                               const bool ignoreFarPMs,
                               const float ignoreDistance2,
                               const int iterations,
                               float scale,
                               const float scaleReductionFactor,
                               const float comp_r0,
                               const float comp_a,                               
                               const float comp_b,
                               const float comp_lam2,
                               const float* pmx,
                               const float* pmy,
                               int numPMs,
                               int lrfSizePerPM,                                   
                               const float* const d_lrfData,
                               const float* const d_eventsData,
                               int numEvents,
                               float* d_recX,
                               float* d_recY,
                               float* d_recEnergy,
                               float* d_chi2)
{
  //setting up shared memory
  int activeThreads = blockDim.x * blockDim.y;

  extern __shared__ float shared[];  //these data are shared withing a thread block - that is for all points of the grid (one event!)
  float* PMx = (float*) &shared;
  float* PMy = (float*) &PMx[numPMs];
  float* signal = (float*) &PMy[numPMs];
  int* nodeIX = (int*) &signal[numPMs];
  int* nodeIY = (int*) &nodeIX[activeThreads];
  float* nodeChi2 = (float*) &nodeIY[activeThreads];
  float* nodeEnergy = (float*) &nodeChi2[activeThreads];
    //for ML only
  float* nodeProb = (float*) &nodeEnergy[activeThreads];

  __shared__ float Xoffset;
  __shared__ float Yoffset;
  __shared__ float Xoffset0; //will be kept unchanged, for dynamic passives by distance
  __shared__ float Yoffset0; //will be kept unchanged, for dynamic passives by distance

  int ievent = blockIdx.x + gridDim.x * blockIdx.y;      //cuda GRID is used to select an event - below word "grid" refers to the grid of XYs positions
  int threadID = threadIdx.x + threadIdx.y * blockDim.x; //block is used to scan XY (each node - one thread)
                                                         //threadIdx.x is related to offset in X from Xcenter of the grid
                                                         //threadIdx.y is related to offset in Y from Ycenter of the grid

  //to do!!! case when numPMs>numtreads in block
  if (threadID<numPMs)
    { //in this block _only_: threadID is PMs index
      PMx[threadID] = pmx[threadID];
      PMy[threadID] = pmy[threadID];

      signal[threadID] = d_eventsData[ievent*(numPMs+2) + threadID]; //buffer contains PM signals and XY offset
    }

  if (threadID == 0)
    { //0th tread sets the center of the grid
      Xoffset = d_eventsData[ievent*(numPMs+2) + numPMs];  //offsetX;
      Xoffset0 = Xoffset;
      Yoffset = d_eventsData[ievent*(numPMs+2) + numPMs+1];//offsetY;
      Yoffset0 = Yoffset;
    }

  __syncthreads(); //need to synchronize - all shared input data are ready

  int activePMs;

  //starting multigrid iterations
  for (int iter=0; iter<iterations; iter++)
   {
     float X = Xoffset + scale * (-0.5*(blockDim.x - 1)+ threadIdx.x); //coordinates of this node (Thread)
     float Y = Yoffset + scale * (-0.5*(blockDim.y - 1)+ threadIdx.y);

     if (ievent<numEvents)
       {
         activePMs = 0;
         //since we cannot save lrfs in a fast tmp storage, collecting all statistics in parralel
         //energy - SUM sig(i) / SUM LRF(i)
         float sumsig = 0;
         float sumLRF = 0;
         //probability for ML: SUM sig(i)*ln{LRF(i)*E} - LRF(i)*E = SUM   sig(i)*ln(LRF(i)) + sig(i)*ln(E) - LRF(i)*E
         float sumsigLnLRF = 0;
         //for chi2:  SUM   {sig(i) - LRF(i)*E)^2 / LRF(i)E  =
         //           SUM   sig2/lrf/E -2*sig +lrf*E
         float sumsig2overLRF = 0;

         bool isBadNode = false; //can be set to true if range to one of PMs larger than the defininition range of its LRF; or energy < 0
         for (int ipm=0; ipm<numPMs; ipm++)
           {
             float tsig = signal[ipm];
             if (ignoreLowSignalPMs)
                if (tsig < ignoreThresholdLow || tsig > ignoreThresholdHigh)
                 {
                  //signal < threshold, ignoring this PM
                  continue;
                 }
             if (ignoreFarPMs)
               {
                 float stX  = (PMx[ipm] - Xoffset0);
                 float stY  = (PMy[ipm] - Yoffset0);
                 float r2 = stX*stX + stY*stY; // r  = sqrtf(r);
                 //distance2 from this PM center to the start position of the search
                 if (r2 > ignoreDistance2)
                  {
                   //too far from this PM center, ignoring this PM
                   continue;
                  }
               }

             float dx = d_lrfData[lrfSizePerPM*ipm + lrfSizePerPM-1]; //LRFs are defined in the range from 0 to dx
             float offX  = (X - PMx[ipm]);
             float offY  = (Y - PMy[ipm]);
             float r = offX*offX + offY*offY;
             r  = sqrtf(r); //distance to the PM center            
             if (r > dx)
               {
                 //outside! cannot use this thread (node) at all
                 isBadNode = true;
                 break;
               }

             activePMs++; //this one is active
             sumsig += tsig;

             //applying compression
             float delta_r = r - comp_r0;
             r =  comp_b + delta_r*comp_a - sqrtf(delta_r*delta_r + comp_lam2);
             if (r<0.0) r = 0.0;

             delta_r = dx - comp_r0;
             dx =  comp_b + delta_r*comp_a - sqrtf(delta_r*delta_r + comp_lam2);
           // if (dx<0.0) dx = 0.0;


             //LRF calculation
             if (r == dx) dx -= 1.0e-7;
             int nint = lrfSizePerPM-4; // lrfSizePerPM = number of intervals +3 + 1
             float xi = r/dx*nint;
             int ix = (int)xi;
             float xf = xi-ix;

             float a0, a1, a2;
             float p0, p1, p2, p3;

             a0 = d_lrfData[lrfSizePerPM*ipm + ix];
             a1 = d_lrfData[lrfSizePerPM*ipm + ix + 1];
             float c2 = d_lrfData[lrfSizePerPM*ipm + ix + 2];
             a2 = a0 + a1 + c2;  // c0 + c1 + c2
             a1 = a1 + a1 + a1;  // 3*c1
             p0 = a2 + a1;       // c0 + 4*c1 + c2
             p2 = a2 - a1;       // c0 - 2*c1 + c2
             p2 = p2 + p2 + p2;  // 3*c0 - 6*c1 + 3*c2
             p1 = c2 - a0;       // c2 - c0
             p1 = p1 + p1 + p1;  // 3*c2 - 3*c0
             p3 = a0 + a0 - a1 - p2 + d_lrfData[lrfSizePerPM*ipm + ix + 3]; // -c0 + 3*c1 - 3*c2 + c3

             float lrf = (p0 + xf*(p1 + xf*(p2 + xf*p3)))/6.;

             //have lrf now
             if (lrf <= 0)
               { //bad lrf
                 isBadNode = true;
                 break;
               }
             sumLRF += lrf;
             if (mlORchi2 == 0) sumsigLnLRF += tsig * __logf(lrf);
             sumsig2overLRF += tsig * tsig / lrf;

           } //end cycle by PMs

         //can calculate energy ("naive" approach)
         float energy = 1.0;
         if (sumLRF > 0 ) energy = sumsig/sumLRF;
         if (energy<1.0e-10) isBadNode = true;

         //can calculate probability and chi2
         //storing results in shared memory
         if (isBadNode)
           {
             if (mlORchi2 == 0) nodeProb[threadID] = -1.0e10;
             nodeChi2[threadID] = 1.0e10;
           }
         else
           {
             if (mlORchi2 == 0) nodeProb[threadID] = sumsigLnLRF  +  sumsig * __logf(energy)  -  sumLRF*energy;
             nodeChi2[threadID] = sumsig2overLRF/energy -2*sumsig + sumLRF*energy;
           }

         nodeIX[threadID] = threadIdx.x;
         nodeIY[threadID] = threadIdx.y;
         nodeEnergy[threadID] = energy;
       }
     else
       {
         //could happen for num events >65535
         //wrong event index - we dont care, these data are not sent to host
         //only important: these threads still can see all __syncthreads()
       }

     __syncthreads(); //to prepare node data

     //looking for the best location - compare nodes always in pairs - using shared memory
      int Nodes = activeThreads;    //activeThreads = blockDim.x * blockDim.y
      do
              {
                int idx = Nodes / 2;
                //---

                if (mlORchi2 == 0) //ML
                  {
                    if (threadID < idx && nodeProb[threadID] < nodeProb[threadID + idx])
                      {
                       nodeProb[threadID] = nodeProb[threadID + idx];
                       nodeIX[threadID] = nodeIX[threadID + idx];
                       nodeIY[threadID] = nodeIY[threadID + idx];
                       nodeEnergy[threadID] = nodeEnergy[threadID + idx];
                       nodeChi2[threadID] = nodeChi2[threadID + idx];
                      }

                    if (threadID == 0)
                     if (Nodes % 2 != 0)  //for the cases when Nodes is odd number
                      {
                        if (nodeProb[0] < nodeProb[Nodes-1]) //all other nodes are already covered
                          {
                           nodeProb[0] = nodeProb[Nodes-1];
                           nodeIX[0] = nodeIX[Nodes-1];
                           nodeIY[0] = nodeIY[Nodes-1];
                           nodeEnergy[0] = nodeEnergy[Nodes-1];
                           nodeChi2[0] = nodeChi2[Nodes-1];
                          }
                      }
                  }
                else
                  {
                    if (threadID < idx && nodeChi2[threadID] > nodeChi2[threadID + idx])
                      {
                       nodeIX[threadID] = nodeIX[threadID + idx];
                       nodeIY[threadID] = nodeIY[threadID + idx];
                       nodeEnergy[threadID] = nodeEnergy[threadID + idx];
                       nodeChi2[threadID] = nodeChi2[threadID + idx];
                      }

                    if (threadID == 0)
                     if (Nodes % 2 != 0)  //for the cases when Nodes is odd number
                      {
                        if (nodeChi2[0] > nodeChi2[Nodes-1]) //all other nodes are already covered
                          {
                           nodeIX[0] = nodeIX[Nodes-1];
                           nodeIY[0] = nodeIY[Nodes-1];
                           nodeEnergy[0] = nodeEnergy[Nodes-1];
                           nodeChi2[0] = nodeChi2[Nodes-1];
                          }
                      }
                  }


                //---
                Nodes = idx;
                __syncthreads();
              }
            while (Nodes>1);

     // thread index 0 contains info on to the best fit location with the current grid
     if (threadID == 0)
       {
         Xoffset +=  scale * (-0.5*(blockDim.x - 1)+ nodeIX[0]); //new gid center - for the next iteration
         Yoffset +=  scale * (-0.5*(blockDim.y - 1)+ nodeIY[0]);
       }
     __syncthreads();


     //preparing scale for the next iteration
     scale /= scaleReductionFactor;
   }//iteration end

  //reporting back reconstructed event data
  if (threadID == 0 && ievent<numEvents)
      {
        //reporting results
        d_recX[ievent] = Xoffset;
        d_recY[ievent] = Yoffset;
        d_recEnergy[ievent] = nodeEnergy[0];
        int df = activePMs -4;  //-1 -2XY -1energy
        if (df < 1) df = 1;
        d_chi2[ievent] = nodeChi2[0] / df;
      }
}

/*
__global__ void kernelFreeform(const bool mlORchi2,
                               const bool ignoreLowSignalPMs,
                               const float ignoreThreshold,
                               const int iterations,
                               float scale,
                               const float scaleReductionFactor,                   
                               int numPMs,
                               int lrfSizePerPM,
                               const int nintx,
                               const int ninty,
                               const float* const d_eventsData,
                               int numEvents,
                               float* d_recX,
                               float* d_recY,
                               float* d_recEnergy,
                               float* d_chi2,
                               float* d_probability)

{
  //setting up shared memory
  int activeThreads = blockDim.x * blockDim.y;

  extern __shared__ float shared[];  //these data are shared withing a thread block - that is for all points of the grid (one event!)
    //grid location
  int* nodeIX = (int*) &shared;
  int* nodeIY = (int*) &nodeIX[activeThreads];
  float* nodeChi2 = (float*) &nodeIY[activeThreads];
  float* nodeEnergy = (float*) &nodeChi2[activeThreads];
    //PM
  float* signal = (float*) &nodeEnergy[activeThreads];
    //used only if ML: probability
  float* nodeProb = (float*) &signal[numPMs]; //will be non-zero only for ML!

  __shared__ float Xoffset;
  __shared__ float Yoffset;

  int ievent = blockIdx.x + gridDim.x * blockIdx.y;      //cuda GRID is used to select an event - below word "grid" refers to the grid of XYs positions
  int threadID = threadIdx.x + threadIdx.y * blockDim.x; //block is used to scan XY (each node - one thread)
                                                         //threadIdx.x is related to offset in X from Xcenter of the grid
                                                         //threadIdx.y is related to offset in Y from Ycenter of the grid

  //to do!!! case when numPMs>numtreads in block
  if (threadID<numPMs)
    { //in this block _only_: threadID is PMs index
      signal[threadID] = d_eventsData[ievent*(numPMs+2) + threadID]; //buffer: signals of all active PMs +XY offset
    }

  if (threadID == 0)
    { //0th tread sets the center of the grid
      Xoffset = d_eventsData[ievent*(numPMs+2) + numPMs];  //offsetX;
      Yoffset = d_eventsData[ievent*(numPMs+2) + numPMs+1];//offsetX;
    }

  __syncthreads(); //need to synchronize - all shared input data are ready

 // float tmp1=-1, tmp2=-1; //test var
  int activePMs;

  //starting multigrid iterations
  for (int iter=0; iter<iterations; iter++)
   {
     activePMs = 0;
     float X = Xoffset + scale * (-0.5*(blockDim.x - 1)+ threadIdx.x); //coordinates of this node (Thread)
     float Y = Yoffset + scale * (-0.5*(blockDim.y - 1)+ threadIdx.y);

     if (ievent<numEvents)
       {
         //since we cannot save lrfs in a fast tmp storage, collecting all statistics in parralel
         //energy: SUM sig(i) / SUM LRF(i)
         float sumsig = 0;
         float sumLRF = 0;
         //probability for ML: SUM sig(i)*ln{LRF(i)*E} - LRF(i)*E = SUM   sig(i)*ln(LRF(i)) + sig(i)*ln(E) - LRF(i)*E
         float sumsigLnLRF = 0;
         //chi2:  SUM   {sig(i) - LRF(i)*E)^2 / LRF(i)E  =
         //       SUM   sig2/lrf/E -2*sig +lrf*E
         float sumsig2overLRF = 0;

         bool isBadNode = false; //can be set to true if range to one of PMs larger than the defininition range of its LRF; or energy < 0
         for (int ipm=0; ipm<numPMs; ipm++)
           {
             float xl = d_LRF[lrfSizePerPM*ipm + lrfSizePerPM-4];
             float xr = d_LRF[lrfSizePerPM*ipm + lrfSizePerPM-3];
             float yl = d_LRF[lrfSizePerPM*ipm + lrfSizePerPM-2];
             float yr = d_LRF[lrfSizePerPM*ipm + lrfSizePerPM-1];

             if (X<xl || X>xr)
               {
                 //outside! cannot use this thread (node) at all
                 isBadNode = true;

                 break;
               }
             if (Y<yl || Y>yr)
               {
                 //outside! cannot use this thread (node) at all
                 isBadNode = true;
                 break;
               }

             float dx = xr - xl;
             float dy = yr - yl;

             float tsig = signal[ipm];
             if (ignoreLowSignalPMs)
                if (tsig < ignoreThreshold)
                  {
                    //signal < threshold, ignoring this PM
                    continue;
                  }
             activePMs++; //this one is active

             sumsig += tsig;

             //LRF calculation
             float xi = (X-xl)/dx*nintx;
             int ix = (int)xi;
             float xf = xi-ix;
             float xff = 1.0 - xf;

             float yi = (Y-yl)/dy*ninty;
             int iy = (int)yi;
             float yf = yi-iy;
             float yff = 1.0 - yf;

             float xx[4], yy[4];

             xx[0] = xff*xff*xff;
             xx[1] = xf*xf*(xf + xf + xf - 6.0) + 4.0;
             xx[2] = xf*(xf*(-xf - xf - xf + 3.0) + 3.0) + 1.;
             xx[3] = xf*xf*xf;

             yy[0] = yff*yff*yff;
             yy[1] = yf*yf*(yf + yf + yf - 6.0) + 4.;
             yy[2] = yf*(yf*(-yf - yf - yf + 3.0) + 3.0) + 1.0;
             yy[3] = yf*yf*yf;

             float lrf = 0.0;
             int nbasx = nintx + 3;
             int k = iy*nbasx+ix; // current 2D base function
             for (int jy=0; jy<4; jy++)
               {
                  for (int jx=0; jx<4; jx++)
                    {
                      lrf += d_LRF[lrfSizePerPM*ipm + k] * xx[jx] * yy[jy];
                      k++;
                    }
                  k += nbasx  -4;
               }
             lrf /= 36.0;

       //      if (threadID == 0 && ipm == 0) tmp1 = k;//lrf;
       //      if (threadID == 0 && ipm == 1) tmp2 = lrf;//lrf;

             //----have lrf now----
             if (lrf <= 0)
               { //bad lrf
                 isBadNode = true;
                 break;
               }

             sumLRF += lrf;
             if (mlORchi2 == 0) sumsigLnLRF += tsig * __logf(lrf);  //ML
             sumsig2overLRF += tsig * tsig / lrf;                   //LS

           } //end cycle by PMs

         //can calculate energy ("naive" approach)
         float energy = 1.0;
         if (sumLRF > 0 ) energy = sumsig/sumLRF;
         if (energy<1.0e-10) isBadNode = true;

         //can calculate probability and chi2
         //storing results in shared memory
         if (isBadNode)
           {
             if (mlORchi2 == 0) nodeProb[threadID] = -1.0e10;
             nodeChi2[threadID] = 1.0e10;
           }
         else
           {
             if (mlORchi2 == 0) nodeProb[threadID] = sumsigLnLRF  +  sumsig * __logf(energy)  -  sumLRF*energy;
             nodeChi2[threadID] = sumsig2overLRF/energy -2*sumsig + sumLRF*energy;
           }

         nodeIX[threadID] = threadIdx.x;
         nodeIY[threadID] = threadIdx.y;
         nodeEnergy[threadID] = energy;         
       }
     else
       {
         //could happen for num events >65535
         //wrong event index - we dont care, these data are not sent to host
         //only important: these threads still can see all __syncthreads()
       }

     __syncthreads(); //to prepare node data

     //looking for the best location - compare nodes always in pairs - using shared memory
      int Nodes = activeThreads;    //activeThreads = blockDim.x * blockDim.y
      do
        {
          int idx = Nodes / 2;
          //---

          if (mlORchi2 == 0) //ML
            {
              if (threadID < idx && nodeProb[threadID] < nodeProb[threadID + idx])
                {
                 nodeProb[threadID] = nodeProb[threadID + idx];
                 nodeIX[threadID] = nodeIX[threadID + idx];
                 nodeIY[threadID] = nodeIY[threadID + idx];
                 nodeEnergy[threadID] = nodeEnergy[threadID + idx];
                 nodeChi2[threadID] = nodeChi2[threadID + idx];
                }

              if (threadID == 0)
               if (Nodes % 2 != 0)  //for the cases when Nodes is odd number
                {
                  if (nodeProb[0] < nodeProb[Nodes-1]) //all other nodes are already covered
                    {
                     nodeProb[0] = nodeProb[Nodes-1];
                     nodeIX[0] = nodeIX[Nodes-1];
                     nodeIY[0] = nodeIY[Nodes-1];
                     nodeEnergy[0] = nodeEnergy[Nodes-1];
                     nodeChi2[0] = nodeChi2[Nodes-1];
                    }
                }
            }
          else
            {
              if (threadID < idx && nodeChi2[threadID] > nodeChi2[threadID + idx])
                {
                 nodeIX[threadID] = nodeIX[threadID + idx];
                 nodeIY[threadID] = nodeIY[threadID + idx];
                 nodeEnergy[threadID] = nodeEnergy[threadID + idx];
                 nodeChi2[threadID] = nodeChi2[threadID + idx];
                }

              if (threadID == 0)
               if (Nodes % 2 != 0)  //for the cases when Nodes is odd number
                {
                  if (nodeChi2[0] > nodeChi2[Nodes-1]) //all other nodes are already covered
                    {
                     nodeIX[0] = nodeIX[Nodes-1];
                     nodeIY[0] = nodeIY[Nodes-1];
                     nodeEnergy[0] = nodeEnergy[Nodes-1];
                     nodeChi2[0] = nodeChi2[Nodes-1];
                    }
                }
            }


          //---
          Nodes = idx;
          __syncthreads();
        }
      while (Nodes>1);

     // thread index 0 contains info on to the best fit location with the current grid
     if (threadID == 0)
       {
         Xoffset +=  scale * (-0.5*(blockDim.x - 1)+ nodeIX[0]); //new gid center - for the next iteration
         Yoffset +=  scale * (-0.5*(blockDim.y - 1)+ nodeIY[0]);
       }
     __syncthreads();


     //preparing scale for the next iteration
     scale /= scaleReductionFactor;
   }//iteration end

  //reporting back reconstructed event data
  if (threadID == 0 && ievent<numEvents)
    {
      //reporting results
      d_recX[ievent] = Xoffset;
      d_recY[ievent] = Yoffset;
      d_recEnergy[ievent] = nodeEnergy[0];
      int df = activePMs - 4; //-1 -2XY -1energy
      if (df < 1) df = 1;
      d_chi2[ievent] = nodeChi2[0] / df;
      if (mlORchi2 == 0) d_probability[ievent] = nodeProb[0];
    }
}
*/


//same as Freeform, but LRFs are given in local coordinates of the sensors
__global__ void kernelXY(const bool mlORchi2,
                               const bool ignoreLowSignalPMs,
                               const float ignoreThresholdLow,
                               const float ignoreThresholdHigh,
                         const bool ignoreFarPMs,
                         const float ignoreDistance2,
                         const float* pmx,
                         const float* pmy,
                               const int iterations,
                               float scale,
                               const float scaleReductionFactor,
                               int numPMs,
                               int lrfSizePerPM,
                               const int nintx,
                               const int ninty,
                               const float* const d_lrfData,
                               const float* const d_eventsData,
                               int numEvents,
                               float* d_recX,
                               float* d_recY,
                               float* d_recEnergy,
                               float* d_chi2,
                               float* d_probability)

{
  //setting up shared memory
  int activeThreads = blockDim.x * blockDim.y;    
  extern __shared__ float shared[];  //these data are shared withing a thread block - that is for all points of the grid (one event!)
    //PM centers
 float* PMx = (float*) &shared;
 float* PMy = (float*) &PMx[numPMs];
    //grid location
  //int* nodeIX = (int*) &shared;
  int* nodeIX = (int*) &PMy[numPMs];
  int* nodeIY = (int*) &nodeIX[activeThreads];
  float* nodeChi2 = (float*) &nodeIY[activeThreads];
  float* nodeEnergy = (float*) &nodeChi2[activeThreads];
    //PM
  float* signal = (float*) &nodeEnergy[activeThreads];
    //used only if ML: probability
  float* nodeProb = (float*) &signal[numPMs]; //will be non-zero only for ML!

  __shared__ float Xoffset;
  __shared__ float Yoffset;
  __shared__ float Xoffset0; //will be kept unchanged, for dynamic passives by distance
  __shared__ float Yoffset0; //will be kept unchanged, for dynamic passives by distance

  int ievent = blockIdx.x + gridDim.x * blockIdx.y;      //cuda GRID is used to select an event - below word "grid" refers to the grid of XYs positions
  int threadID = threadIdx.x + threadIdx.y * blockDim.x; //block is used to scan XY (each node - one thread)
                                                         //threadIdx.x is related to offset in X from Xcenter of the grid
                                                         //threadIdx.y is related to offset in Y from Ycenter of the grid

  //to do!!! case when numPMs>numtreads in block
  if (threadID<numPMs)
    { //in this block _only_: threadID is PMs index
      signal[threadID] = d_eventsData[ievent*(numPMs+2) + threadID]; //buffer: signals of all active PMs +XY offset
    }

  if (threadID == 0)
    { //0th tread sets the center of the grid
      Xoffset = d_eventsData[ievent*(numPMs+2) + numPMs];  //offsetX;
      Xoffset0 = Xoffset;
      Yoffset = d_eventsData[ievent*(numPMs+2) + numPMs+1];//offsetY;
      Yoffset0 = Yoffset;
    }

  __syncthreads(); //need to synchronize - all shared input data are ready

 // float tmp1=-1, tmp2=-1; //test var
  int activePMs;

  //starting multigrid iterations
  for (int iter=0; iter<iterations; iter++)
   {
     activePMs = 0;
     float X = Xoffset + scale * (-0.5*(blockDim.x - 1)+ threadIdx.x); //coordinates of this node (Thread)
     float Y = Yoffset + scale * (-0.5*(blockDim.y - 1)+ threadIdx.y);

     if (ievent<numEvents)
       {
         //since we cannot save lrfs in a fast tmp storage, collecting all statistics in parralel
         //energy: SUM sig(i) / SUM LRF(i)
         float sumsig = 0;
         float sumLRF = 0;
         //probability for ML: SUM sig(i)*ln{LRF(i)*E} - LRF(i)*E = SUM   sig(i)*ln(LRF(i)) + sig(i)*ln(E) - LRF(i)*E
         float sumsigLnLRF = 0;
         //chi2:  SUM   {sig(i) - LRF(i)*E)^2 / LRF(i)E  =
         //       SUM   sig2/lrf/E -2*sig +lrf*E
         float sumsig2overLRF = 0;

         bool isBadNode = false; //can be set to true if range to one of PMs larger than the defininition range of its LRF; or energy < 0
         for (int ipm=0; ipm<numPMs; ipm++)
           {
             //doing backtransform
             float deltax = d_lrfData[lrfSizePerPM*ipm + lrfSizePerPM-9];
             float deltay = d_lrfData[lrfSizePerPM*ipm + lrfSizePerPM-8];
             float sinphi = d_lrfData[lrfSizePerPM*ipm + lrfSizePerPM-7];
             float cosphi = d_lrfData[lrfSizePerPM*ipm + lrfSizePerPM-6];
             float flip =   d_lrfData[lrfSizePerPM*ipm + lrfSizePerPM-5];

             float xtmp = X + deltax;
             float ytmp = Y + deltay;
             float       XL =  xtmp*cosphi - ytmp*sinphi;
             float YL;
             if (flip<0) YL =  xtmp*sinphi + ytmp*cosphi;  //no flip
             else        YL = -xtmp*sinphi - ytmp*cosphi;  //with flip

             float xl = d_lrfData[lrfSizePerPM*ipm + lrfSizePerPM-4];
             float xr = d_lrfData[lrfSizePerPM*ipm + lrfSizePerPM-3];
             float yl = d_lrfData[lrfSizePerPM*ipm + lrfSizePerPM-2];
             float yr = d_lrfData[lrfSizePerPM*ipm + lrfSizePerPM-1];

             if (XL<xl || XL>xr)
               {
                 //outside! cannot use this thread (node) at all
                 isBadNode = true;

                 break;
               }
             if (YL<yl || YL>yr)
               {
                 //outside! cannot use this thread (node) at all
                 isBadNode = true;
                 break;
               }

             float dx = xr - xl;
             float dy = yr - yl;

             float tsig = signal[ipm];
             if (ignoreLowSignalPMs)
                if (tsig < ignoreThresholdLow || tsig > ignoreThresholdHigh)
                  {
                    //signal < threshold, ignoring this PM
                    continue;
                  }
             if (ignoreFarPMs)
               {
                 float stX  = (PMx[ipm] - Xoffset0);
                 float stY  = (PMy[ipm] - Yoffset0);
                 float r2 = stX*stX + stY*stY; // r  = sqrtf(r);
                 //distance2 from this PM center to the start position of the search
                 if (r2 > ignoreDistance2)
                  {
                   //too far from this PM center, ignoring this PM
                   continue;
                  }
               }

             activePMs++; //this one is active

             sumsig += tsig;

             //LRF calculation
             float xi = (XL-xl)/dx*nintx;
             int ix = (int)xi;
             float xf = xi-ix;
             float xff = 1.0 - xf;

             float yi = (YL-yl)/dy*ninty;
             int iy = (int)yi;
             float yf = yi-iy;
             float yff = 1.0 - yf;

             float xx[4], yy[4];

             xx[0] = xff*xff*xff;
             xx[1] = xf*xf*(xf + xf + xf - 6.0) + 4.0;
             xx[2] = xf*(xf*(-xf - xf - xf + 3.0) + 3.0) + 1.;
             xx[3] = xf*xf*xf;

             yy[0] = yff*yff*yff;
             yy[1] = yf*yf*(yf + yf + yf - 6.0) + 4.;
             yy[2] = yf*(yf*(-yf - yf - yf + 3.0) + 3.0) + 1.0;
             yy[3] = yf*yf*yf;

             float lrf = 0.0;
             int nbasx = nintx + 3;
             int k = iy*nbasx+ix; // current 2D base function
             for (int jy=0; jy<4; jy++)
               {
                  for (int jx=0; jx<4; jx++)
                    {
                      lrf += d_lrfData[lrfSizePerPM*ipm + k] * xx[jx] * yy[jy];
                      k++;
                    }
                  k += nbasx  -4;
               }
             lrf /= 36.0;

       //      if (threadID == 0 && ipm == 0) tmp1 = k;//lrf;
       //      if (threadID == 0 && ipm == 1) tmp2 = lrf;//lrf;

             //----have lrf now----
             if (lrf <= 0)
               { //bad lrf
                 isBadNode = true;
                 break;
               }

             sumLRF += lrf;
             if (mlORchi2 == 0) sumsigLnLRF += tsig * __logf(lrf);  //ML
             sumsig2overLRF += tsig * tsig / lrf;                   //LS

           } //end cycle by PMs

         //can calculate energy ("naive" approach)
         float energy = 1.0;
         if (sumLRF > 0 ) energy = sumsig/sumLRF;
         if (energy<1.0e-10) isBadNode = true;

         //can calculate probability and chi2
         //storing results in shared memory
         if (isBadNode)
           {
             if (mlORchi2 == 0) nodeProb[threadID] = -1.0e10;
             nodeChi2[threadID] = 1.0e10;
           }
         else
           {
             if (mlORchi2 == 0) nodeProb[threadID] = sumsigLnLRF  +  sumsig * __logf(energy)  -  sumLRF*energy;
             nodeChi2[threadID] = sumsig2overLRF/energy -2*sumsig + sumLRF*energy;
           }

         nodeIX[threadID] = threadIdx.x;
         nodeIY[threadID] = threadIdx.y;
         nodeEnergy[threadID] = energy;
       }
     else
       {
         //could happen for num events >65535
         //wrong event index - we dont care, these data are not sent to host
         //only important: these threads still can see all __syncthreads()
       }

     __syncthreads(); //to prepare node data

     //looking for the best location - compare nodes always in pairs - using shared memory
      int Nodes = activeThreads;    //activeThreads = blockDim.x * blockDim.y
      do
        {
          int idx = Nodes / 2;
          //---

          if (mlORchi2 == 0) //ML
            {
              if (threadID < idx && nodeProb[threadID] < nodeProb[threadID + idx])
                {
                 nodeProb[threadID] = nodeProb[threadID + idx];
                 nodeIX[threadID] = nodeIX[threadID + idx];
                 nodeIY[threadID] = nodeIY[threadID + idx];
                 nodeEnergy[threadID] = nodeEnergy[threadID + idx];
                 nodeChi2[threadID] = nodeChi2[threadID + idx];
                }

              if (threadID == 0)
               if (Nodes % 2 != 0)  //for the cases when Nodes is odd number
                {
                  if (nodeProb[0] < nodeProb[Nodes-1]) //all other nodes are already covered
                    {
                     nodeProb[0] = nodeProb[Nodes-1];
                     nodeIX[0] = nodeIX[Nodes-1];
                     nodeIY[0] = nodeIY[Nodes-1];
                     nodeEnergy[0] = nodeEnergy[Nodes-1];
                     nodeChi2[0] = nodeChi2[Nodes-1];
                    }
                }
            }
          else
            {
              if (threadID < idx && nodeChi2[threadID] > nodeChi2[threadID + idx])
                {
                 nodeIX[threadID] = nodeIX[threadID + idx];
                 nodeIY[threadID] = nodeIY[threadID + idx];
                 nodeEnergy[threadID] = nodeEnergy[threadID + idx];
                 nodeChi2[threadID] = nodeChi2[threadID + idx];
                }

              if (threadID == 0)
               if (Nodes % 2 != 0)  //for the cases when Nodes is odd number
                {
                  if (nodeChi2[0] > nodeChi2[Nodes-1]) //all other nodes are already covered
                    {
                     nodeIX[0] = nodeIX[Nodes-1];
                     nodeIY[0] = nodeIY[Nodes-1];
                     nodeEnergy[0] = nodeEnergy[Nodes-1];
                     nodeChi2[0] = nodeChi2[Nodes-1];
                    }
                }
            }


          //---
          Nodes = idx;
          __syncthreads();
        }
      while (Nodes>1);

     // thread index 0 contains info on to the best fit location with the current grid
     if (threadID == 0)
       {
         Xoffset +=  scale * (-0.5*(blockDim.x - 1)+ nodeIX[0]); //new gid center - for the next iteration
         Yoffset +=  scale * (-0.5*(blockDim.y - 1)+ nodeIY[0]);
       }
     __syncthreads();


     //preparing scale for the next iteration
     scale /= scaleReductionFactor;
   }//iteration end

  //reporting back reconstructed event data
  if (threadID == 0 && ievent<numEvents)
    {
      //reporting results
      d_recX[ievent] = Xoffset;
      d_recY[ievent] = Yoffset;
      d_recEnergy[ievent] = nodeEnergy[0];
      int df = activePMs -4; //-1 -2XY -1energy
      if (df < 1) df = 1;
      d_chi2[ievent] = nodeChi2[0] / df;
      if (mlORchi2 == 0) d_probability[ievent] = nodeProb[0];
    }
}

__global__ void kernelComposite(const bool mlORchi2,
                                const bool ignoreLowSigPMs,
                                const float ignoreThresholdLow,
                                const float ignoreThresholdHigh,
                                const bool ignoreFarPMs,
                                const float ignoreDistance2,
                                const int iterations,
                                float scale,
                                const float scaleReductionFactor,
                                const float comp_r0,
                                const float comp_a,
                                const float comp_b,
                                const float comp_lam2,
                                const bool fCompressed,
                                const float* pmx,
                                const float* pmy,
                                int numPMs,
                                int lrfFloatsPerPM,
                                int lrfFloatsAxialPerPM,
                                int nintx,
                                int ninty,
                                const float* const d_lrfData,
                                const float* const d_eventsData,
                                int numEvents,
                                float* d_recX,
                                float* d_recY,
                                float* d_recEnergy,
                                float* d_chi2,
                                float* d_probability)
{
  //setting up shared memory
  int activeThreads = blockDim.x * blockDim.y;

  extern __shared__ float shared[];  //these data are shared withing a thread block - that is for all points of the grid (one event!)
  float* PMx = (float*) &shared;
  float* PMy = (float*) &PMx[numPMs];
  float* signal = (float*) &PMy[numPMs];
  int* nodeIX = (int*) &signal[numPMs];
  int* nodeIY = (int*) &nodeIX[activeThreads];
  float* nodeChi2 = (float*) &nodeIY[activeThreads];
  float* nodeEnergy = (float*) &nodeChi2[activeThreads];
    //for ML only
  float* nodeProb = (float*) &nodeEnergy[activeThreads];

  __shared__ float Xoffset;
  __shared__ float Yoffset;
  __shared__ float Xoffset0; //will be kept unchanged, for dynamic passives by distance
  __shared__ float Yoffset0; //will be kept unchanged, for dynamic passives by distance

  int ievent = blockIdx.x + gridDim.x * blockIdx.y;      //cuda GRID is used to select an event - below word "grid" refers to the grid of XYs positions
  int threadID = threadIdx.x + threadIdx.y * blockDim.x; //block is used to scan XY (each node - one thread)
                                                         //threadIdx.x is related to offset in X from Xcenter of the grid
                                                         //threadIdx.y is related to offset in Y from Ycenter of the grid

  //to do!!! case when numPMs>numtreads in block
  if (threadID<numPMs)
    { //in this block _only_: threadID is PMs index
      PMx[threadID] = pmx[threadID];
      PMy[threadID] = pmy[threadID];

      signal[threadID] = d_eventsData[ievent*(numPMs+2) + threadID]; //buffer contains PM signals and XY offset
    }

  if (threadID == 0)
    { //0th tread sets the center of the grid
      Xoffset = d_eventsData[ievent*(numPMs+2) + numPMs];  //offsetX;
      Xoffset0 = Xoffset;
      Yoffset = d_eventsData[ievent*(numPMs+2) + numPMs+1];//offsetY;
      Yoffset0 = Yoffset;
    }

  __syncthreads(); //need to synchronize - all shared input data are ready

  int activePMs;

  //starting multigrid iterations
  for (int iter=0; iter<iterations; iter++)
   {
     float X = Xoffset + scale * (-0.5*(blockDim.x - 1)+ threadIdx.x); //coordinates of this node (Thread)
     float Y = Yoffset + scale * (-0.5*(blockDim.y - 1)+ threadIdx.y);

     if (ievent<numEvents)
       {
         activePMs = 0;
         //since we cannot save lrfs in a fast tmp storage, collecting all statistics in parralel
         //energy - SUM sig(i) / SUM LRF(i)
         float sumsig = 0;
         float sumLRF = 0;
         //probability for ML: SUM sig(i)*ln{LRF(i)*E} - LRF(i)*E = SUM   sig(i)*ln(LRF(i)) + sig(i)*ln(E) - LRF(i)*E
         float sumsigLnLRF = 0;
         //for chi2:  SUM   {sig(i) - LRF(i)*E)^2 / LRF(i)E  =
         //           SUM   sig2/lrf/E -2*sig +lrf*E
         float sumsig2overLRF = 0;

         bool isBadNode = false; //can be set to true if range to one of PMs larger than the defininition range of its LRF; or energy < 0
         for (int ipm=0; ipm<numPMs; ipm++)
           {
             float tsig = signal[ipm];

             //check if this PM is skipped..
             if (ignoreLowSigPMs)       //..by signal
                if (tsig < ignoreThresholdLow || tsig > ignoreThresholdHigh)
                 {
                  //signal < threshold, ignoring this PM
                  continue;
                 }
             if (ignoreFarPMs)          //..by distance
               {
                 float stX  = (PMx[ipm] - Xoffset0);
                 float stY  = (PMy[ipm] - Yoffset0);
                 float r2 = stX*stX + stY*stY; // r  = sqrtf(r);
                 //distance2 from this PM center to the start position of the search
                 if (r2 > ignoreDistance2)
                  {
                   //too far from this PM center, ignoring this PM
                   continue;
                  }
               }

             //check if both LRF are defined and applying conversions
             //..first axial
             float dr = d_lrfData[lrfFloatsPerPM*ipm + lrfFloatsAxialPerPM-1]; //LRFs are defined in the range from 0 to dr
             float offX  = (X - PMx[ipm]);
             float offY  = (Y - PMy[ipm]);
             float r = offX*offX + offY*offY;
             r  = sqrtf(r); //distance to the PM center
             if (r > dr)
               {
                 //outside! cannot use this thread (node) at all
                 isBadNode = true;
                 break;
               }
               //applying compression
             if (fCompressed)
               {
                 float delta_r = r - comp_r0;
                 r =  comp_b + delta_r*comp_a - sqrtf(delta_r*delta_r + comp_lam2);
                 if (r<0.0) r = 0.0;

                 delta_r = dr - comp_r0;
                 dr =  comp_b + delta_r*comp_a - sqrtf(delta_r*delta_r + comp_lam2);
                 // if (dr<0.0) dr = 0.0;
               }
             //..then xy
               //doing backtransform
             float deltax = d_lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-9];
             float deltay = d_lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-8];
             float sinphi = d_lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-7];
             float cosphi = d_lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-6];
             float flip =   d_lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-5];

             float xtmp = X + deltax;
             float ytmp = Y + deltay;
             float       XL =  xtmp*cosphi - ytmp*sinphi;
             float YL;
             if (flip<0) YL =  xtmp*sinphi + ytmp*cosphi;  //no flip
             else        YL = -xtmp*sinphi - ytmp*cosphi;  //with flip

             float xl = d_lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-4];
             float xr = d_lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-3];
             float yl = d_lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-2];
             float yr = d_lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-1];

             //if (threadID == 0) printf("PM#: %d \n XY:%f, %f\n FXL,YL: %f %f\nRange minmax X: %f %f\nY: %f %f\n",ipm, X, Y, XL, YL, xl,xr,yl,yr);

             if (XL<xl || XL>xr)
               { //outside! cannot use this thread (node) at all
                 isBadNode = true;
                 break;
               }
             if (YL<yl || YL>yr)
               { //outside! cannot use this thread (node) at all
                 isBadNode = true;
                 break;
               }

             //if we reach this point, this PM is active!
             activePMs++;
             sumsig += tsig;


             //LRF calculation..
             //..axial part
             if (r == dr) dr -= 1.0e-7;
             int nint = lrfFloatsAxialPerPM-4; // lrfFloatsAxialPerPM = number of intervals +3 + 1
             float xi = r/dr*nint;
             int ix = (int)xi;
             float xf = xi-ix;

             float a0, a1, a2;
             float p0, p1, p2, p3;

             a0 = d_lrfData[lrfFloatsPerPM*ipm + ix];
             a1 = d_lrfData[lrfFloatsPerPM*ipm + ix + 1];
             float c2 = d_lrfData[lrfFloatsPerPM*ipm + ix + 2];
             a2 = a0 + a1 + c2;  // c0 + c1 + c2
             a1 = a1 + a1 + a1;  // 3*c1
             p0 = a2 + a1;       // c0 + 4*c1 + c2
             p2 = a2 - a1;       // c0 - 2*c1 + c2
             p2 = p2 + p2 + p2;  // 3*c0 - 6*c1 + 3*c2
             p1 = c2 - a0;       // c2 - c0
             p1 = p1 + p1 + p1;  // 3*c2 - 3*c0
             p3 = a0 + a0 - a1 - p2 + d_lrfData[lrfFloatsPerPM*ipm + ix + 3]; // -c0 + 3*c1 - 3*c2 + c3

             float lrfAxial = (p0 + xf*(p1 + xf*(p2 + xf*p3)))/6.;
//             if (lrfAxial <= 0)
//               { //bad lrf
//                 isBadNode = true;
//                 break;
//               }

             //..now xy LRFs
             float dx = xr - xl;
             float dy = yr - yl;
             xi = (XL-xl)/dx*nintx;
             ix = (int)xi;
             xf = xi-ix;
             float xff = 1.0 - xf;

             float yi = (YL-yl)/dy*ninty;
             int iy = (int)yi;
             float yf = yi-iy;
             float yff = 1.0 - yf;

             float xx[4], yy[4];

             xx[0] = xff*xff*xff;
             xx[1] = xf*xf*(xf + xf + xf - 6.0) + 4.0;
             xx[2] = xf*(xf*(-xf - xf - xf + 3.0) + 3.0) + 1.;
             xx[3] = xf*xf*xf;

             yy[0] = yff*yff*yff;
             yy[1] = yf*yf*(yf + yf + yf - 6.0) + 4.;
             yy[2] = yf*(yf*(-yf - yf - yf + 3.0) + 3.0) + 1.0;
             yy[3] = yf*yf*yf;

             float lrfXY = 0.0;
             int nbasx = nintx + 3;
             int k = iy*nbasx+ix; // current 2D base function
             for (int jy=0; jy<4; jy++)
               {
                  for (int jx=0; jx<4; jx++)
                    {
                      lrfXY += d_lrfData[lrfFloatsPerPM*ipm + lrfFloatsAxialPerPM + k] * xx[jx] * yy[jy];
                      k++;
                    }
                  k += nbasx  -4;
               }
             lrfXY /= 36.0;

             float lrf = lrfAxial + lrfXY;
             //note - it seems now as it is implemented in the LRF library, individual LRFs can be negative, only important that sum LRFs is > 0
             if (lrf <= 0)
               { //bad lrf
                 isBadNode = true;
                 break;
               }


             //--------------- have lrf now ----------------------
             //printf("Pm number: %d, XY: %f,%f, LRF=%f\n", ipm, X, Y, lrf);

             sumLRF += lrf;
             if (mlORchi2 == 0) sumsigLnLRF += tsig * __logf(lrf);
             sumsig2overLRF += tsig * tsig / lrf;

           } //end cycle by PMs

         //can calculate energy ("naive" approach)
         float energy = 1.0;
         if (sumLRF > 0 ) energy = sumsig/sumLRF;
         if (energy<1.0e-10) isBadNode = true;

         //can calculate probability and chi2
         //storing results in shared memory
         if (isBadNode)
           {
             if (mlORchi2 == 0) nodeProb[threadID] = -1.0e10;
             nodeChi2[threadID] = 1.0e10;
           }
         else
           {
             if (mlORchi2 == 0) nodeProb[threadID] = sumsigLnLRF  +  sumsig * __logf(energy)  -  sumLRF*energy;
             nodeChi2[threadID] = sumsig2overLRF/energy -2*sumsig + sumLRF*energy;
           }

         nodeIX[threadID] = threadIdx.x;
         nodeIY[threadID] = threadIdx.y;
         nodeEnergy[threadID] = energy;
       }
     else
       {
         //could happen for num events >65535
         //wrong event index - we dont care, these data are not sent to host
         //only important: these threads still can see all __syncthreads()
       }

     __syncthreads(); //to prepare node data

     //looking for the best location - compare nodes always in pairs - using shared memory
      int Nodes = activeThreads;    //activeThreads = blockDim.x * blockDim.y
      do
              {
                int idx = Nodes / 2;
                //---

                if (mlORchi2 == 0) //ML
                  {
                    if (threadID < idx && nodeProb[threadID] < nodeProb[threadID + idx])
                      {
                       nodeProb[threadID] = nodeProb[threadID + idx];
                       nodeIX[threadID] = nodeIX[threadID + idx];
                       nodeIY[threadID] = nodeIY[threadID + idx];
                       nodeEnergy[threadID] = nodeEnergy[threadID + idx];
                       nodeChi2[threadID] = nodeChi2[threadID + idx];
                      }

                    if (threadID == 0)
                     if (Nodes % 2 != 0)  //for the cases when Nodes is odd number
                      {
                        if (nodeProb[0] < nodeProb[Nodes-1]) //all other nodes are already covered
                          {
                           nodeProb[0] = nodeProb[Nodes-1];
                           nodeIX[0] = nodeIX[Nodes-1];
                           nodeIY[0] = nodeIY[Nodes-1];
                           nodeEnergy[0] = nodeEnergy[Nodes-1];
                           nodeChi2[0] = nodeChi2[Nodes-1];
                          }
                      }
                  }
                else
                  {
                    if (threadID < idx && nodeChi2[threadID] > nodeChi2[threadID + idx])
                      {
                       nodeIX[threadID] = nodeIX[threadID + idx];
                       nodeIY[threadID] = nodeIY[threadID + idx];
                       nodeEnergy[threadID] = nodeEnergy[threadID + idx];
                       nodeChi2[threadID] = nodeChi2[threadID + idx];
                      }

                    if (threadID == 0)
                     if (Nodes % 2 != 0)  //for the cases when Nodes is odd number
                      {
                        if (nodeChi2[0] > nodeChi2[Nodes-1]) //all other nodes are already covered
                          {
                           nodeIX[0] = nodeIX[Nodes-1];
                           nodeIY[0] = nodeIY[Nodes-1];
                           nodeEnergy[0] = nodeEnergy[Nodes-1];
                           nodeChi2[0] = nodeChi2[Nodes-1];
                          }
                      }
                  }


                //---
                Nodes = idx;
                __syncthreads();
              }
            while (Nodes>1);

     // thread index 0 contains info on to the best fit location with the current grid
     if (threadID == 0)
       {
         Xoffset +=  scale * (-0.5*(blockDim.x - 1)+ nodeIX[0]); //new gid center - for the next iteration
         Yoffset +=  scale * (-0.5*(blockDim.y - 1)+ nodeIY[0]);
       }
     __syncthreads();


     //preparing scale for the next iteration
     scale /= scaleReductionFactor;
   }//iteration end

  //reporting back reconstructed event data
  if (threadID == 0 && ievent<numEvents)
      {
        //reporting results
        d_recX[ievent] = Xoffset;
        d_recY[ievent] = Yoffset;
        d_recEnergy[ievent] = nodeEnergy[0];
        int df = activePMs -4;  //-1 -2XY -1energy
        if (df < 1) df = 1;
        d_chi2[ievent] = nodeChi2[0] / df;
        if (mlORchi2 == 0) d_probability[ievent] = nodeProb[0];

        //printf("XY: %f,%f, Energy:%f\nChi2:%f\nActive PMs:%d\n", Xoffset, Yoffset, nodeEnergy[0], nodeChi2[0]/df, activePMs);
      }
}
