# 构图
# prefix=/home/cy/data/siftsmall
# ./build/tests/test_build_roargraph --data_type float --dist l2 \
# --base_data_path ${prefix}/siftsmall_base.fbin \
# --sampled_query_data_path ${prefix}/siftsmall_learn.fbin \
# --projection_index_save_path ${prefix}/siftsmall_roar.index \
# --learn_base_nn_path ${prefix}/siftsmall_learn_base_gt100.ibin \
# --M_sq 100 --M_pjbp 35 --L_pjpq 500 -T 64

# 搜索（memory）
# num_threads=1
# topk=10
# prefix=/home/cy/data/siftsmall
# ./build/tests/test_search_roargraph --data_type float --dist l2 \
# --base_data_path ${prefix}/siftsmall_base.fbin \
# --projection_index_save_path ${prefix}/siftsmall_roar.index \
# --gt_path ${prefix}/siftsmall_query_base_gt100.ibin  \
# --query_path ${prefix}/siftsmall_query.fbin \
# --L_pq 10 20 30 40 50 \
# --k ${topk} -T ${num_threads} \
# --evaluation_save_path ${prefix}/test_search_top${topk}_T${num_threads}.csv


# 搜索（disk）
# num_threads=1
# topk=10
# prefix=/home/cy/data/siftsmall
# ./build/tests/test_search_roargraph_on_disk --data_type float --dist l2 \
# --base_data_path ${prefix}/siftsmall_base.fbin \
# --projection_index_save_path ${prefix}/siftsmall_roar.index \
# --gt_path ${prefix}/siftsmall_query_base_gt100.ibin  \
# --query_path ${prefix}/siftsmall_query.fbin \
# --L_pq 10 20 30 40 50 \
# --k ${topk} -T ${num_threads} \
# --evaluation_save_path ${prefix}/test_search_top${topk}_T${num_threads}.csv \
# --disk_index_save_path /mnt/nvme0n1p1/cy_disk_index_only_graph

# 搜索（disk）(use sq)
num_threads=1
topk=10
prefix=/home/cy/data/siftsmall
./build/tests/test_search_roargraph_on_disk_use_sq --data_type float --dist l2 \
--base_data_path ${prefix}/siftsmall_base.fbin \
--projection_index_save_path ${prefix}/siftsmall_roar.index \
--gt_path ${prefix}/siftsmall_query_base_gt100.ibin  \
--query_path ${prefix}/siftsmall_query.fbin \
--L_pq 10 20 30 40 50 \
--k ${topk} -T ${num_threads} \
--evaluation_save_path ${prefix}/test_search_top${topk}_T${num_threads}.csv \
--disk_index_save_path /mnt/nvme0n1p1/cy_disk_index_only_graph