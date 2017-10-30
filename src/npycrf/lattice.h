#pragma once
#include <vector>
#include "common.h"
#include "crf/crf.h"
#include "npylm/npylm.h"

namespace npycrf {
	namespace lattice {
		void _init_alpha(double*** &alpha, int size, int max_word_length);
		void _delete_alpha(double*** &alpha, int size, int max_word_length);
	}
	class Lattice {
	public:
		npylm::NPYLM* _npylm;
		crf::CRF* _crf;
		id* _word_ids;
		id** _substring_word_id_cache;
		double*** _alpha;		// 前向き確率
		double*** _normalized_alpha;	// 正規化後の前向き確率
		double**** _pw_h;		// キャッシュ
		double* _log_z;			// 正規化定数
		double* _backward_sampling_table;
		int*** _viterbi_backward;
		int _max_word_length;
		int _max_sentence_length;
		double _lambda_0;
		Lattice(npylm::NPYLM* npylm, crf::CRF* crf, double lambda_0, int max_word_length, int max_sentence_length);
		~Lattice();
		id get_substring_word_id_at_t_k(Sentence* sentence, int t, int k);
		void allocate_arrays(int max_word_length, int max_sentence_length);
		void delete_arrays();
		void sum_alpha_t_k_j(Sentence* sentence, int t, int k, int j, double*** normalized_alpha);
		void forward_filtering(Sentence* sentence, double*** normalized_alpha);
		void backward_sampling(Sentence* sentence, std::vector<int> &segments, double*** backward_alpha);
		void sample_backward_k_and_j(Sentence* sentence, int t, int next_word_length, int &sampled_k, int &sampled_j, double*** backward_alpha);
		void blocked_gibbs(Sentence* sentence, std::vector<int> &segments, bool normalize = true);
		void viterbi_argmax_alpha_t_k_j(Sentence* sentence, int t, int k, int j);
		void viterbi_forward(Sentence* sentence);
		void viterbi_argmax_backward_k_and_j_to_eos(Sentence* sentence, int t, int next_word_length, int &argmax_k, int &argmax_j);
		void viterbi_backward(Sentence* sentence, std::vector<int> &segments);
		void viterbi_decode(Sentence* sentence, std::vector<int> &segments);
		double compute_forward_probability(Sentence* sentence, bool normalize = true);
	};
} // namespace npylm