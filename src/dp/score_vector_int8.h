/****
DIAMOND protein aligner
Copyright (C) 2013-2019 Benjamin Buchfink <buchfink@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
****/

#pragma once
#include "score_vector.h"

namespace DISPATCH_ARCH {

#if ARCH_ID == 2

template<>
struct score_vector<int8_t>
{

	score_vector() :
		data_(::SIMD::_mm256_set1_epi8(SCHAR_MIN))
	{}

	explicit score_vector(__m256i data) :
		data_(data)
	{}

	explicit score_vector(int8_t x) :
		data_(::SIMD::_mm256_set1_epi8(x))
	{}

	explicit score_vector(int x) :
		data_(::SIMD::_mm256_set1_epi8(x))
	{}

	explicit score_vector(const int8_t* s) :
		data_(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(s)))
	{ }

	explicit score_vector(const uint8_t* s) :
		data_(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(s)))
	{ }

	score_vector(unsigned a, __m256i seq)
	{
		const __m256i* row_lo = reinterpret_cast<const __m256i*>(&score_matrix.matrix8_low()[a << 5]);
		const __m256i* row_hi = reinterpret_cast<const __m256i*>(&score_matrix.matrix8_high()[a << 5]);

		__m256i high_mask = _mm256_slli_epi16(_mm256_and_si256(seq, ::SIMD::_mm256_set1_epi8('\x10')), 3);
		__m256i seq_low = _mm256_or_si256(seq, high_mask);
		__m256i seq_high = _mm256_or_si256(seq, _mm256_xor_si256(high_mask, ::SIMD::_mm256_set1_epi8('\x80')));

		__m256i r1 = _mm256_load_si256(row_lo);
		__m256i r2 = _mm256_load_si256(row_hi);

		__m256i s1 = _mm256_shuffle_epi8(r1, seq_low);
		__m256i s2 = _mm256_shuffle_epi8(r2, seq_high);
		data_ = _mm256_or_si256(s1, s2);
	}

	score_vector operator+(const score_vector& rhs) const
	{
		return score_vector(_mm256_adds_epi8(data_, rhs.data_));
	}

	score_vector operator-(const score_vector& rhs) const
	{
		return score_vector(_mm256_subs_epi8(data_, rhs.data_));
	}

	score_vector& operator+=(const score_vector& rhs) {
		data_ = _mm256_adds_epi8(data_, rhs.data_);
		return *this;
	}

	score_vector& operator-=(const score_vector& rhs)
	{
		data_ = _mm256_subs_epi8(data_, rhs.data_);
		return *this;
	}

	score_vector& operator &=(const score_vector& rhs) {
		data_ = _mm256_and_si256(data_, rhs.data_);
		return *this;
	}

	score_vector& operator++() {
		data_ = _mm256_adds_epi8(data_, ::SIMD::_mm256_set1_epi8(1));
		return *this;
	}

	friend score_vector blend(const score_vector &v, const score_vector &w, const score_vector &mask) {
		return score_vector(_mm256_blendv_epi8(v.data_, w.data_, mask.data_));
	}

	score_vector operator==(const score_vector &v) {
		return score_vector(_mm256_cmpeq_epi8(data_, v.data_));
	}

	friend uint32_t cmp_mask(const score_vector &v, const score_vector &w) {
		return (uint32_t)_mm256_movemask_epi8(_mm256_cmpeq_epi8(v.data_, w.data_));
	}

	int operator [](unsigned i) const
	{
		return *(((uint8_t*)&data_) + i);
	}

	void set(unsigned i, uint8_t v)
	{
		*(((uint8_t*)&data_) + i) = v;
	}

	score_vector& max(const score_vector& rhs)
	{
		data_ = _mm256_max_epi8(data_, rhs.data_);
		return *this;
	}

	score_vector& min(const score_vector& rhs)
	{
		data_ = _mm256_min_epi8(data_, rhs.data_);
		return *this;
	}

	friend score_vector max(const score_vector& lhs, const score_vector& rhs)
	{
		return score_vector(_mm256_max_epi8(lhs.data_, rhs.data_));
	}

	friend score_vector min(const score_vector& lhs, const score_vector& rhs)
	{
		return score_vector(_mm256_min_epi8(lhs.data_, rhs.data_));
	}

	void store(int8_t* ptr) const
	{
		_mm256_storeu_si256((__m256i*)ptr, data_);
	}

	friend std::ostream& operator<<(std::ostream& s, score_vector v)
	{
		int8_t x[32];
		v.store(x);
		for (unsigned i = 0; i < 32; ++i)
			printf("%3i ", (int)x[i]);
		return s;
	}

	__m256i data_;

};

template<>
struct ScoreTraits<score_vector<int8_t>>
{
	enum { CHANNELS = 32 };
	typedef ::DISPATCH_ARCH::SIMD::Vector<int8_t> Vector;
	typedef int8_t Score;
	typedef uint8_t Unsigned;
	typedef uint32_t Mask;
	struct TraceMask {
		static uint64_t make(uint32_t vmask, uint32_t hmask) {
			return (uint64_t)vmask << 32 | (uint64_t)hmask;
		}
		static uint64_t vmask(int channel) {
			return (uint64_t)1 << (channel + 32);
		}
		static uint64_t hmask(int channel) {
			return (uint64_t)1 << channel;
		}
		uint64_t gap;
		uint64_t open;
	};
	static score_vector<int8_t> zero() {
		return score_vector<int8_t>();
	}
	static constexpr int8_t max_score() {
		return SCHAR_MAX;
	}
	static int int_score(int8_t s)
	{
		return (int)s - SCHAR_MIN;
	}
	static constexpr int max_int_score() {
		return SCHAR_MAX - SCHAR_MIN;
	}
	static constexpr int8_t zero_score() {
		return SCHAR_MIN;
	}
	static void saturate(score_vector<int8_t>& v) {}
};

#elif defined(__SSE4_1__)

template<>
struct score_vector<int8_t>
{

	score_vector():
		data_(::SIMD::_mm_set1_epi8(SCHAR_MIN))
	{}

	explicit score_vector(__m128i data):
		data_(data)
	{}

	explicit score_vector(int8_t x):
		data_(::SIMD::_mm_set1_epi8(x))
	{}

	explicit score_vector(int x):
		data_(::SIMD::_mm_set1_epi8(x))
	{}

	explicit score_vector(const int8_t* s) :
		data_(_mm_loadu_si128(reinterpret_cast<const __m128i*>(s)))
	{ }

	explicit score_vector(const uint8_t* s) :
		data_(_mm_loadu_si128(reinterpret_cast<const __m128i*>(s)))
	{ }

	score_vector(unsigned a, __m128i seq)
	{
		const __m128i* row = reinterpret_cast<const __m128i*>(&score_matrix.matrix8()[a << 5]);

		__m128i high_mask = _mm_slli_epi16(_mm_and_si128(seq, ::SIMD::_mm_set1_epi8('\x10')), 3);
		__m128i seq_low = _mm_or_si128(seq, high_mask);
		__m128i seq_high = _mm_or_si128(seq, _mm_xor_si128(high_mask, ::SIMD::_mm_set1_epi8('\x80')));

		__m128i r1 = _mm_load_si128(row);
		__m128i r2 = _mm_load_si128(row + 1);
		__m128i s1 = _mm_shuffle_epi8(r1, seq_low);
		__m128i s2 = _mm_shuffle_epi8(r2, seq_high);
		data_ = _mm_or_si128(s1, s2);
	}

	score_vector operator+(const score_vector &rhs) const
	{
		return score_vector(_mm_adds_epi8(data_, rhs.data_));
	}

	score_vector operator-(const score_vector &rhs) const
	{
		return score_vector(_mm_subs_epi8(data_, rhs.data_));
	}

	score_vector& operator+=(const score_vector& rhs) {
		data_ = _mm_adds_epi8(data_, rhs.data_);
		return *this;
	}

	score_vector& operator-=(const score_vector& rhs)
	{
		data_ = _mm_subs_epi8(data_, rhs.data_);
		return *this;
	}

	score_vector& operator &=(const score_vector& rhs) {
		data_ = _mm_and_si128(data_, rhs.data_);
		return *this;
	}

	score_vector& operator++() {
		data_ = _mm_adds_epi8(data_, ::SIMD::_mm_set1_epi8(1));
		return *this;
	}

	friend score_vector blend(const score_vector &v, const score_vector &w, const score_vector &mask) {
		return score_vector(_mm_blendv_epi8(v.data_, w.data_, mask.data_));
	}

	score_vector operator==(const score_vector &v) {
		return score_vector(_mm_cmpeq_epi8(data_, v.data_));
	}

	friend uint32_t cmp_mask(const score_vector &v, const score_vector &w) {
		return _mm_movemask_epi8(_mm_cmpeq_epi8(v.data_, w.data_));
	}

	int operator [](unsigned i) const
	{
		return *(((uint8_t*)&data_) + i);
	}

	void set(unsigned i, uint8_t v)
	{
		*(((uint8_t*)&data_) + i) = v;
	}

	score_vector& max(const score_vector &rhs)
	{
		data_ = _mm_max_epi8(data_, rhs.data_);
		return *this;
	}

	score_vector& min(const score_vector &rhs)
	{
		data_ = _mm_min_epi8(data_, rhs.data_);
		return *this;
	}

	friend score_vector max(const score_vector& lhs, const score_vector &rhs)
	{
		return score_vector(_mm_max_epi8(lhs.data_, rhs.data_));
	}

	friend score_vector min(const score_vector& lhs, const score_vector &rhs)
	{
		return score_vector(_mm_min_epi8(lhs.data_, rhs.data_));
	}

	void store(int8_t *ptr) const
	{
		_mm_storeu_si128((__m128i*)ptr, data_);
	}

	friend std::ostream& operator<<(std::ostream &s, score_vector v)
	{
		int8_t x[16];
		v.store(x);
		for (unsigned i = 0; i < 16; ++i)
			printf("%3i ", (int)x[i]);
		return s;
	}

	__m128i data_;

};

template<>
struct ScoreTraits<score_vector<int8_t>>
{
	enum { CHANNELS = 16 };
	typedef ::DISPATCH_ARCH::SIMD::Vector<int8_t> Vector;
	typedef int8_t Score;
	typedef uint8_t Unsigned;
	typedef uint16_t Mask;
	struct TraceMask {
		static uint32_t make(uint32_t vmask, uint32_t hmask) {
			return vmask << 16 | hmask;
		}
		static uint32_t vmask(int channel) {
			return 1 << (channel + 16);
		}
		static uint32_t hmask(int channel) {
			return 1 << channel;
		}
		uint32_t gap;
		uint32_t open;
	};
	static score_vector<int8_t> zero() {
		return score_vector<int8_t>();
	}
	static constexpr int8_t max_score() {
		return SCHAR_MAX;
	}
	static int int_score(int8_t s)
	{
		return (int)s - SCHAR_MIN;
	}
	static constexpr int max_int_score() {
		return SCHAR_MAX - SCHAR_MIN;
	}
	static constexpr int8_t zero_score() {
		return SCHAR_MIN;
	}
	static void saturate(score_vector<int8_t>& v) {}
};

#endif

}

#ifdef __SSE4_1__

static inline DISPATCH_ARCH::score_vector<int8_t> load_sv(const int8_t *x) {
	return DISPATCH_ARCH::score_vector<int8_t>(x);
}

static inline DISPATCH_ARCH::score_vector<int8_t> load_sv(const uint8_t *x) {
	return DISPATCH_ARCH::score_vector<int8_t>(x);
}

#endif