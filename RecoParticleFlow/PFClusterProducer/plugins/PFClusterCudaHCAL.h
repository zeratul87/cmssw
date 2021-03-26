#ifndef PFClusterCudaHCAL_h
#define PFClusterCudaHCAL_h
//#include <thrust/host_vector.h>
//#include <thrust/device_vector.h>
#include <Eigen/Dense>

namespace PFClusterCudaHCAL { 
  
  void initializeCudaConstants(float h_showerSigma = 0.f,
                               const float (&h_recHitEnergyNormEB_vec)[4] = {0.f,0.f,0.f,0.f},
                               const float (&h_recHitEnergyNormEE_vec)[7] = {0.f,0.f,0.f,0.f,0.f,0.f,0.f},
                               float h_minFracToKeep = 0.,
                               const float (&h_seedEThresholdEB_vec)[4] = {0.f,0.f,0.f,0.f}, 
                               const float (&h_seedEThresholdEE_vec)[7] = {0.f,0.f,0.f,0.f,0.f,0.f,0.f},
                               float h_seedPt2ThresholdEB = 0.,
                               float h_seedPt2hresholdEE = 0.,
                               const float (&h_topoEThresholdEB_vec)[4] = {0.f,0.f,0.f,0.f},
                               const float (&h_topoEThresholdEE_vec)[7] = {0.f,0.f,0.f,0.f,0.f,0.f,0.f},
                               int   h_nNeigh = 0,
                               int   h_maxSize = 100
                               );


  void PFRechitToPFCluster_HCALV1(size_t size, 
				float* pfrh_x,
				float* pfrh_y, 
				float* pfrh_z,	
				float* pfrh_energy,	
				float* pfrh_pt2, 
				int* pfrh_isSeed, 
				int* pfrh_topoId, 
				int* pfrh_layer, 
				int* pfrh_depth, 
				int* neigh8_Ind, 				
				int* neigh4_Ind, 				
				float* pfrhfrac, 
				int* pfrhind, 
				int* pcrhind,
				float* pcrhfracind
				);

void PFRechitToPFCluster_HCALV2(size_t size, 
				const float* __restrict__ pfrh_x,
				const float* __restrict__ pfrh_y,
				const float* __restrict__ pfrh_z,
				const double* __restrict__ pfrh_energy,
				const double* __restrict__ pfrh_pt2,
				int* pfrh_isSeed, 
				int* pfrh_topoId, 
				const int* __restrict__ pfrh_layer, 
				const int* __restrict__ pfrh_depth, 
				const int* __restrict__ neigh8_Ind, 				
				const int* __restrict__ neigh4_Ind, 				
				int* pcrhind,
				float* pcrhfracind,
				float* fracSum,
				int* rhCount,
				float* timer = nullptr 
                );

void PFRechitToPFCluster_HCAL_serialize(size_t size, 
				const float* __restrict__ pfrh_x,
				const float* __restrict__ pfrh_y,
				const float* __restrict__ pfrh_z,
				const double* __restrict__ pfrh_energy,
				const double* __restrict__ pfrh_pt2,
				int* pfrh_isSeed, 
				int* pfrh_topoId, 
				const int* __restrict__ pfrh_layer, 
				const int* __restrict__ pfrh_depth, 
				const int* __restrict__ neigh8_Ind, 				
				const int* __restrict__ neigh4_Ind, 				
				int* pcrhind,
				float* pcrhfracind,
				float* fracSum,
				int* rhCount,
				float* timer = nullptr 
				);

void PFRechitToPFCluster_HCAL_serialize_seedingParallel(size_t size, 
				const float* __restrict__ pfrh_x,
				const float* __restrict__ pfrh_y,
				const float* __restrict__ pfrh_z,
				const double* __restrict__ pfrh_energy,
				const double* __restrict__ pfrh_pt2,
				int* pfrh_isSeed, 
				int* pfrh_topoId, 
				const int* __restrict__ pfrh_layer, 
				const int* __restrict__ pfrh_depth, 
				const int* __restrict__ neigh8_Ind, 				
				const int* __restrict__ neigh4_Ind, 				
				int* pcrhind,
				float* pcrhfracind,
				float* fracSum,
				int* rhCount,
				float* timer = nullptr 
				);

void PFRechitToPFCluster_HCAL_serialize_topoParallel(size_t size, 
				const float* __restrict__ pfrh_x,
				const float* __restrict__ pfrh_y,
				const float* __restrict__ pfrh_z,
				const double* __restrict__ pfrh_energy,
				const double* __restrict__ pfrh_pt2,
				int* pfrh_isSeed, 
				int* pfrh_topoId, 
				const int* __restrict__ pfrh_layer, 
				const int* __restrict__ pfrh_depth, 
				const int* __restrict__ neigh8_Ind, 				
				const int* __restrict__ neigh4_Ind, 				
				int* pcrhind,
				float* pcrhfracind,
				float* fracSum,
				int* rhCount
				);

void PFRechitToPFCluster_HCAL_serialize_step1Parallel(size_t size, 
				const float* __restrict__ pfrh_x,
				const float* __restrict__ pfrh_y,
				const float* __restrict__ pfrh_z,
				const double* __restrict__ pfrh_energy,
				const double* __restrict__ pfrh_pt2,
				int* pfrh_isSeed, 
				int* pfrh_topoId, 
				const int* __restrict__ pfrh_layer, 
				const int* __restrict__ pfrh_depth, 
				const int* __restrict__ neigh8_Ind, 				
				const int* __restrict__ neigh4_Ind, 				
				int* pcrhind,
				float* pcrhfracind,
				float* fracSum,
				int* rhCount
				);

void PFRechitToPFCluster_HCAL_serialize_step2Parallel(size_t size, 
				const float* __restrict__ pfrh_x,
				const float* __restrict__ pfrh_y,
				const float* __restrict__ pfrh_z,
				const double* __restrict__ pfrh_energy,
				const double* __restrict__ pfrh_pt2,
				int* pfrh_isSeed, 
				int* pfrh_topoId, 
				const int* __restrict__ pfrh_layer, 
				const int* __restrict__ pfrh_depth, 
				const int* __restrict__ neigh8_Ind, 				
				const int* __restrict__ neigh4_Ind, 				
				int* pcrhind,
				float* pcrhfracind,
				float* fracSum,
				int* rhCount
				);


}  // namespace cudavectors

#endif  // ClusterCudaECAL_h
