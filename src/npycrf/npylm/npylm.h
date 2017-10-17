#pragma once
#include <vector> 
#include "../common.h"
#include "lm/node.h"
#include "lm/vpylm.h"
#include "lm/hpylm.h"

namespace npylm {
	// character_idsのsubstr_char_t_startからsubstr_char_t_endまでの文字列を<bow>と<eow>で挟んでwrapped_character_idsの先頭に格納
	void wrap_bow_eow(wchar_t const* character_ids, int substr_char_t_start, int substr_char_t_end, wchar_t* wrapped_character_ids);
	double factorial(double n);
	class NPYLM {
	private:
		void _init_cache(int max_word_length, int max_sentence_length);
		void _delete_cache();
		friend class boost::serialization::access;
		template <class Archive>
		void serialize(Archive &archive, unsigned int version);
		void save(boost::archive::binary_oarchive &archive, unsigned int version) const;
		void load(boost::archive::binary_iarchive &archive, unsigned int version);
	public:
		lm::HPYLM* _hpylm;	// 単語n-gram
		lm::VPYLM* _vpylm;	// 文字n-gram

		// 単語unigramノードで新たなテーブルが作られた時はVPYLMからその単語が生成されたと判断し、単語の文字列をVPYLMに追加する
		// その時各文字がVPYLMのどの深さに追加されたかを保存する
		// 単語unigramノードのテーブルごと、単語IDごとに保存する必要がある
		hashmap<id, std::vector<std::vector<int>>> _prev_depth_at_table_of_token;

		hashmap<id, double> _g0_cache;
		hashmap<id, double> _vpylm_g0_cache;
		double* _lambda_for_type;
		double* _pk_vpylm;	// 文字n-gramから長さkの単語が生成される確率
		int _max_word_length;
		int _max_sentence_length;
		double _lambda_a;
		double _lambda_b;
		// 計算高速化用
		double* _hpylm_parent_pw_cache;
		wchar_t* _characters;
		NPYLM(){}
		NPYLM(int max_word_length, 
			int max_sentence_length, 
			double g0, 
			double initial_lambda_a, 
			double initial_lambda_b, 
			double vpylm_beta_stop, 
			double vpylm_beta_pass);
		~NPYLM();
		void allocate_arrays(int max_sentence_length);
		void delete_arrays();
		void set_vpylm_g0(double g0);
		void set_lambda_prior(double a, double b);
		void sample_lambda_with_initial_params();
		bool add_customer_at_time_t(Sentence* sentence, int t);
		void vpylm_add_customers(wchar_t const* character_ids, int substr_char_t_start, int substr_char_t_end, wchar_t* wrapped_character_ids, std::vector<int> &prev_depths);
		bool remove_customer_at_time_t(Sentence* sentence, int t);
		void vpylm_remove_customers(wchar_t const* character_ids, int substr_char_t_start, int substr_char_t_end, wchar_t* wrapped_character_ids, std::vector<int> &prev_depths);
		lm::Node<id>* find_node_by_tracing_back_context_from_time_t(id const* word_ids, int word_ids_length, int word_t_index, bool generate_node_if_needed, bool return_middle_node);
		lm::Node<id>* find_node_by_tracing_back_context_from_time_t(Sentence* sentence, int word_t_index, double* parent_pw_cache, int generate_node_if_needed, bool return_middle_node);
		lm::Node<id>* find_node_by_tracing_back_context_from_time_t(
			wchar_t const* character_ids, int character_ids_length, 
			id const* word_ids, int word_ids_length, 
			int word_t_index, int substr_char_t_start, int substr_char_t_end, 
			double* parent_pw_cache, bool generate_node_if_needed, bool return_middle_node);
		// word_idは既知なので再計算を防ぐ
		double compute_g0_substring_at_time_t(wchar_t const* character_ids, int character_ids_length, int substr_char_t_start, int substr_char_t_end, id word_t_id);
		double compute_poisson_k_lambda(unsigned int k, double lambda);
		double compute_p_k_given_vpylm(int k);
		void sample_hpylm_vpylm_hyperparameters();
		double compute_log_p_w(Sentence* sentence);
		double compute_p_w(Sentence* sentence);
		double compute_p_w_given_h(Sentence* sentence, int word_t_index);
		double compute_p_w_given_h(
			wchar_t const* character_ids, int character_ids_length, 
			id const* word_ids, int word_ids_length, 
			int word_t_index, int substr_char_t_start, int substr_char_t_end);
	};
} // namespace npylm