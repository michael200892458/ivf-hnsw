#ifndef IVF_HNSW_LIB_INDEX_H
#define IVF_HNSW_LIB_INDEX_H

#include <iostream>
#include <fstream>
#include <cstdio>

#include <faiss/ProductQuantizer.h>
#include <faiss/utils.h>

#include <hnswlib/hnswalg.h>
#include "utils.h"

typedef unsigned char uint8_t;
typedef unsigned int idx_t;    ///< all indices are this type

namespace ivfhnsw {
    /** Abstract structure for an index
    *
    * Supports HNSW construction, PQ training, serialization and searching.
    *
    * Currently only asymmetric queries are supported:
    * database-to-database queries are not implemented.
    */
    struct Index
    {
        size_t d;             ///< Vector dimension 
        size_t nc;            ///< Number of centroids
        size_t code_size;     ///< Code size per vector in bytes

        /** HNSW [Y.Malkov] Quantizer **/
        hnswlib::HierarchicalNSW *quantizer;  ///< Quantizer that maps vectors to inverted lists (HNSW [Y.Malkov])

        /** Product quantizers **/
        faiss::ProductQuantizer *pq;
        faiss::ProductQuantizer *norm_pq;

        /** Search Parameters **/
        size_t nprobe = 16;        ///< Number of probes at query time
        size_t max_codes = 10000;  ///< Number of possible key values

        /** Query Table **/
        std::vector<float> query_table;

        std::vector<std::vector<idx_t> > ids;           ///< Inverted lists for indexes
        std::vector<std::vector<uint8_t> > codes;       ///< PQ codes of data
        std::vector<std::vector<uint8_t> > norm_codes;  ///< PQ codes of norms of reconstructed vectors

    protected:
        std::vector<float> norms;           ///< Reconstructed vectors L2 square norms
        std::vector<float> centroid_norms;  ///< Region centroids L2 square norms

    public:
        explicit Index(size_t dim, size_t ncentroids,
                       size_t bytes_per_code, size_t nbits_per_idx);
        virtual ~Index();

        /** Construct from stretch or load the existing quantizer (HNSW) instance
          *
          * if all files exist, quantizer will be loaded, else HNSW will be constructed
          * @param path_data           path to input vectors
          * @param path_info           path to parameters for HNSW
          * @param path_edges          path to edges for HNSW
          * @param M                   minimum number of edges per point, default: 16
          * @param efConstruction      maximum number of observed vertices at once during construction, default: 500
        */
        void buildQuantizer(const char *path_data, const char *path_info, const char *path_edges,
                            int M=16, int efConstruction = 500);

        /** Return the indexes of the k HNSW vertices closest to the query x.
          *
          * @param n           number of input vectors
          * @param x           input vectors to search, size n * d
          * @param labels      output labels of the NNs, size n * k
          * @param k           number of closest vertices to the query x
        */
        void assign (idx_t n, const float *x, idx_t *labels, idx_t k = 1);

        /** Query n vectors of dimension d to the index.
         *
         * Return at most k vectors. If there are not enough results for a
         * query, the result array is padded with -1s.
         *
         * @param x           input vectors to search, size n * d
         * @param labels      output labels of the NNs, size n*k
         * @param distances   output pairwise distances, size n*k
         */
        virtual void search(float *x, idx_t k, float *distances, long *labels) = 0;

        virtual void train_pq(const size_t n, const float *x) = 0;

        virtual void write(const char *path_index) = 0;
        virtual void read(const char *path_index) = 0;

        virtual void compute_centroid_norms();

    protected:
        virtual float fstdistfunc(uint8_t *code);

        //void reconstruct(size_t n, float *x, const float *decoded_residuals, const idx_t *keys) = 0;
        //void compute_residuals(size_t n, const float *x, float *residuals, const idx_t *keys) = 0;
    };
}
#endif //IVF_HNSW_LIB_INDEX_H
