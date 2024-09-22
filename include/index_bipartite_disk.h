#include <fcntl.h>
#include "index_bipartite.h"

// define likely unlikely
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

namespace efanna2e
{
    class FileReader
    {
    public:
        FileReader() {};

        void OpenFile(const std::string &filePath)
        {
            fd_ = open(filePath.c_str(), O_RDONLY | O_DIRECT | O_LARGEFILE);
            if (fd_ == -1)
            {
                throw std::runtime_error("Unable to open file: " + filePath);
            }
            file_path_ = filePath;
        }
        ~FileReader()
        {
            if (fd_ != -1)
            {
                close(fd_);
            }
        }

        // 使用内部缓冲区读取数据
        // void read(size_t offset, size_t size, std::vector<char>& buffer) {
        void read(size_t offset, size_t size, char *buffer)
        {
            // bufferSize_ = buffer.size();
            // if (size > bufferSize_) {
            //     throw std::runtime_error("Requested size exceeds buffer capacity.");
            // }

            if (LIKELY(lseek64(fd_, offset, SEEK_SET) == (off_t)-1))
            {
                throw std::runtime_error("Error seeking file.");
            }

            ssize_t bytesRead = ::read(fd_, buffer, size);
            if (bytesRead == -1)
            {
                throw std::runtime_error("Error reading file.");
            }

            // 实际读取的字节可能少于请求的大小，这里可以根据需要处理
            // return buffer_;
        }

    private:
        int fd_ = -1;
        size_t bufferSize_;
        bool init_ = false;
        std::string file_path_;
        // std::vector<char> buffer_; // 内部缓冲区
    };

    class IndexBipartiteOnDisk : public IndexBipartite
    {
    protected:
        uint32_t disk_index_node_block_size_;
        uint32_t aligned_disk_index_node_block_size_;
        size_t disk_index_head_size_;
        std::vector<void *> disk_index_node_buffers_;
        std::vector<FileReader> disk_readers;

    public:
        explicit IndexBipartiteOnDisk(const size_t dimension, const size_t n, Metric m, Index *initializer) : IndexBipartite(dimension, n, m, initializer)
        {
            return;
        };

        void GenerateDiskIndex(const char *filename)
        {
            std::ofstream out(filename, std::ios::binary | std::ios::out);
            if (!out.is_open())
            {
                throw std::runtime_error("cannot open file");
            }
            out.write((char *)&projection_ep_, sizeof(uint32_t));
            out.write((char *)&u32_nd_, sizeof(uint32_t));

            uint32_t max_degree = 0;
            for (uint32_t i = 0; i < u32_nd_; ++i)
            {
                max_degree = std::max(max_degree, (uint32_t)projection_graph_[i].size());
            }
            std::cout << "max_degree: " << max_degree << std::endl;
            // number of neighbors (4 bytes) + neighbors (4 * n bytes)
            uint32_t each_node_block = sizeof(uint32_t) + sizeof(uint32_t) * max_degree;
            // round up to 4096
            each_node_block = (each_node_block + 4095) & ~4095;

            out.write((char *)&each_node_block, sizeof(uint32_t));
            out.write((char *)std::vector<char>(4096 - 12, 0).data(), 4096 - 12); // align 到 4096B

            for (uint32_t i = 0; i < u32_nd_; ++i)
            {
                uint32_t current_written_size = 0;
                uint32_t nbr_size = projection_graph_[i].size();
                nbr_size = std::min(max_degree, nbr_size);
                out.write((char *)&nbr_size, sizeof(uint32_t));
                current_written_size += 4;
                out.write((char *)projection_graph_[i].data(), sizeof(uint32_t) * nbr_size);
                current_written_size += sizeof(uint32_t) * nbr_size;
                if (current_written_size < each_node_block)
                {
                    // padding zero to the file
                    size_t padding_size = each_node_block - current_written_size;
                    std::vector<char> padding(padding_size, 0);
                    out.write((char *)padding.data(), padding_size);
                }
            }
            std::cout << "In save projection graph for DISK SEARCH to " << std::string(filename) << ", save number of points: " << u32_nd_ << std::endl;
            std::cout << "Each node block size:" << each_node_block << std::endl;
            out.close();
        }

        void PrepareDiskSearch(std::string &disk_file_path, uint32_t num_threads)
        {
            std::ifstream in(disk_file_path, std::ios::binary);
            uint32_t npts;
            in.read((char *)&projection_ep_, sizeof(uint32_t));
            std::cout << "Projection graph, "
                      << "ep: " << projection_ep_ << std::endl;
            in.read((char *)&npts, sizeof(npts));
            nd_ = npts;
            u32_nd_ = npts;

            in.read((char *)&disk_index_node_block_size_, sizeof(uint32_t));
            // round up size to 4096
            // aligned_disk_index_node_block_size_ = (disk_index_node_block_size_ + 4095) & ~4095;
            aligned_disk_index_node_block_size_ = disk_index_node_block_size_;
            disk_readers.resize(num_threads);
            disk_index_node_buffers_.resize(num_threads);
            for (uint32_t i = 0; i < num_threads; ++i)
            {
                disk_readers[i].OpenFile(disk_file_path);
                // disk_index_node_buffers_[i] = (char *) aligned_alloc(4 * 1024, disk_index_node_block_size_);
                if (posix_memalign(&disk_index_node_buffers_[i], 4096, aligned_disk_index_node_block_size_) != 0)
                {
                    perror("posix_memalign failed");
                    exit(-1);
                    // return 1;
                }
                // resize(disk_index_node_block_size_);
            }
            // disk_index_head_size_ = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t);
            // disk_index_head_size_ = 12;
            disk_index_head_size_ = 4096; // align 到 4096B
            // disk_node_nbr_size_ = (uint32_t *) disk_index_node_buffer_.data();
            // disk_cur_vec_data_ = (float *) (disk_index_node_buffer_.data() + 4);
            // disk_node_nbrs_ = (uint32_t *) (disk_index_node_buffer_.data() + 4 + sizeof(float) * dimension_);
            // disk_nbr_vec_data_ = (float *) (disk_index_node_buffer_.data() + 4 + sizeof(float) * dimension_ + 4 * (*disk_node_nbr_size_));
            in.close();
        }

        std::pair<uint32_t, uint32_t> SearchDiskIndex(const float *query, size_t k, size_t &qid, const Parameters &parameters, unsigned *indices, std::vector<float> &res_dists, int thread_no)
        {
            assert(query != nullptr);

            uint32_t L_pq = parameters.Get<uint32_t>("L_pq");
            NeighborPriorityQueue search_queue(L_pq);

            auto &disk_reader = disk_readers[thread_no];
            auto &buffer = disk_index_node_buffers_[thread_no];
            uint32_t *disk_node_nbr_size = (uint32_t *)buffer;
            uint32_t *disk_node_nbrs = (uint32_t *)((char *)buffer + 4);
            assert(buffer != nullptr);

            std::vector<uint32_t> init_ids;
            init_ids.push_back(projection_ep_);

            VisitedList *vl = visited_list_pool_->getFreeVisitedList();
            vl_type *visited_array = vl->mass;
            vl_type visited_array_tag = vl->curV;

            // std::cout << "################" << std::endl;
            // std::cout << "disk_index_head_size_" << disk_index_head_size_ << std::endl;
            // std::cout << "disk_index_node_block_size_" << disk_index_node_block_size_ << std::endl;
            // std::cout << "aligned_disk_index_node_block_size_" << aligned_disk_index_node_block_size_ << std::endl;
            for (auto &id : init_ids)
            {
                // std::cout << "######" << std::endl;
                // std::cout << id << std::endl;
                // std::cout << disk_index_head_size_ + id * disk_index_node_block_size_ << std::endl;
                float distance = distance_->compare(data_bp_ + id * dimension_, query, (unsigned)dimension_);
                Neighbor nn = Neighbor(id, distance, false);
                search_queue.insert(nn);
            }
            uint32_t cmps = 0;
            uint32_t hops = 0;
            while (search_queue.has_unexpanded_node())
            {
                auto cur_check_node = search_queue.closest_unexpanded();
                auto cur_id = cur_check_node.id;

                disk_reader.read(disk_index_head_size_ + (size_t)cur_id * disk_index_node_block_size_, aligned_disk_index_node_block_size_, (char *)buffer);
                uint32_t *cur_nbrs = disk_node_nbrs;
                ++hops;

                // std::cout << projection_graph_[cur_id].size() << std::endl;
                // std::cout << *disk_node_nbr_size << std::endl;

                for (size_t j = 0; j < *disk_node_nbr_size; ++j)
                {
                    // current check node's neighbors
                    uint32_t nbr = *(cur_nbrs + j);
                    // _mm_prefetch((char *)(visited_array + *(cur_nbrs + j + 1)), _MM_HINT_T0);
                    // _mm_prefetch((char *)(disk_nbr_vec_data + (j + 1) * dimension_), _MM_HINT_T0);
                    if (visited_array[nbr] != visited_array_tag)
                    {
                        visited_array[nbr] = visited_array_tag;
                        float distance = distance_->compare(data_bp_ + nbr * dimension_, query, (unsigned)dimension_);
                        ++cmps;
                        search_queue.insert({nbr, distance, false});
                    }
                }
            }
            visited_list_pool_->releaseVisitedList(vl);

            if (unlikely(search_queue.size() < k))
            {
                std::stringstream ss;
                ss << "not enough results: " << search_queue.size() << ", expected: " << k;
                throw std::runtime_error(ss.str());
            }

            for (size_t i = 0; i < k; ++i)
            {
                indices[i] = search_queue[i].id;
                res_dists[i] = search_queue[i].distance;
            }
            return std::make_pair(cmps, hops);
        }
    };
}