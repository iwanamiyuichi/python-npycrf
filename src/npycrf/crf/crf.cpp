#include "crf.h"

namespace npycrf {
	namespace crf {
		CRF::CRF(int feature_unigram_start = -2,
				 int feature_unigram_end = 2,
				 int feature_bigram_start = -2,
				 int feature_bigram_end = 1,
				 int feature_identical_1_start = -2,
				 int feature_identical_1_end = 1,
				 int feature_identical_2_start = -3,
				 int feature_identical_2_end = 1,
				 int num_character_ids)
		{
			bias = 0;
			// NPYCRFではラベルは0か1の2通り
			_w_unigram = new double*[2];
			_w_bigram = new double**[2];
			_w_identical_1 = new double*[2];
			_w_identical_2 = new double*[2];
		}
	}
}