#ifndef _MDR_NEGABINARY_BP_ENCODER_HPP
#define _MDR_NEGABINARY_BP_ENCODER_HPP

#include "BitplaneEncoderInterface.hpp"

namespace MDR {
    // general bitplane encoder that encodes data by block using T_stream type buffer
    template<class T_data, class T_stream>
    class NegaBinaryBPEncoder : public concepts::BitplaneEncoderInterface<T_data> {
    public:
        NegaBinaryBPEncoder(){
            static_assert(std::is_floating_point<T_data>::value, "NegaBinaryBPEncoder: input data must be floating points.");
            static_assert(!std::is_same<T_data, long double>::value, "NegaBinaryBPEncoder: long double is not supported.");
            static_assert(std::is_unsigned<T_stream>::value, "NegaBinaryEncoder: streams must be unsigned integers.");
            static_assert(std::is_integral<T_stream>::value, "NegaBinaryEncoder: streams must be unsigned integers.");
        }

        std::vector<uint8_t *> encode(T_data const * data, int32_t n, int32_t exp, uint8_t num_bitplanes, std::vector<uint32_t>& stream_sizes) const {
            assert(num_bitplanes > 0);
            // leave room for negabinary format
            exp += 1;
            // determine block size based on bitplane integer type
            uint32_t block_size = block_size_based_on_bitplane_int_type<T_stream>();
            std::vector<uint8_t> starting_bitplanes = std::vector<uint8_t>((n - 1)/block_size + 1, 0);
            stream_sizes = std::vector<uint32_t>(num_bitplanes, 0);
            // define fixed point type
            using T_fps = typename std::conditional<std::is_same<T_data, double>::value, int64_t, int32_t>::type;
            using T_fp = typename std::conditional<std::is_same<T_data, double>::value, uint64_t, uint32_t>::type;
            std::vector<uint8_t *> streams;
            for(int i=0; i<num_bitplanes; i++){
                streams.push_back((uint8_t *) malloc(n / UINT8_BITS + sizeof(T_stream)));
            }
            std::vector<T_fp> int_data_buffer(block_size, 0);
            std::vector<T_stream *> streams_pos(streams.size());
            for(int i=0; i<streams.size(); i++){
                streams_pos[i] = reinterpret_cast<T_stream*>(streams[i]);
            }
            T_data const * data_pos = data;
            for(int i=0; i<n - block_size; i+=block_size){
                for(int j=0; j<block_size; j++){
                    T_data cur_data = *(data_pos++);
                    T_data shifted_data = ldexp(cur_data, num_bitplanes - exp);
                    int_data_buffer[j] = binary2negabinary((T_fps) shifted_data);
                }
                encode_block(int_data_buffer.data(), block_size, num_bitplanes, streams_pos);
            }
            // leftover
            {
                int rest_size = n % block_size;
                if(rest_size == 0) rest_size = block_size;
                for(int j=0; j<rest_size; j++){
                    T_data cur_data = *(data_pos++);
                    T_data shifted_data = ldexp(cur_data, num_bitplanes - exp);
                    int_data_buffer[j] = binary2negabinary((T_fps) shifted_data);
                }
                encode_block(int_data_buffer.data(), rest_size, num_bitplanes, streams_pos);
            }
            for(int i=0; i<num_bitplanes; i++){
                stream_sizes[i] = reinterpret_cast<uint8_t*>(streams_pos[i]) - streams[i];
            }
            return streams;
        }

        T_data * decode(const std::vector<uint8_t const *>& streams, int32_t n, int exp, uint8_t num_bitplanes) {
            return progressive_decode(streams, n, exp, 0, num_bitplanes, streams.size());
        }

        // decode the data and record necessary information for progressiveness
        T_data * progressive_decode(const std::vector<uint8_t const *>& streams, int32_t n, int exp, uint8_t starting_bitplane, uint8_t num_bitplanes, int level) {
            uint32_t block_size = block_size_based_on_bitplane_int_type<T_stream>();
            T_data * data = (T_data *) malloc(n * sizeof(T_data));
            if(num_bitplanes == 0){
                memset(data, 0, n * sizeof(T_data));
                return data;
            }
            // leave room for negabinary format
            exp += 1;
            // define fixed point type
            using T_fps = typename std::conditional<std::is_same<T_data, double>::value, int64_t, int32_t>::type;
            using T_fp = typename std::conditional<std::is_same<T_data, double>::value, uint64_t, uint32_t>::type;
            std::vector<T_stream const *> streams_pos(streams.size());
            for(int i=0; i<streams.size(); i++){
                streams_pos[i] = reinterpret_cast<T_stream const *>(streams[i]);
            }
            std::vector<T_fp> int_data_buffer(block_size, 0);
            // decode
            const uint8_t ending_bitplane = starting_bitplane + num_bitplanes;
            T_data * data_pos = data;
            for(int i=0; i<n - block_size; i+=block_size){
                memset(int_data_buffer.data(), 0, block_size * sizeof(T_fp));
                decode_block(streams_pos, block_size, num_bitplanes, int_data_buffer.data());
                for(int j=0; j<block_size; j++){
                    *(data_pos++) = ldexp((T_data) negabinary2binary(int_data_buffer[j]), - ending_bitplane + exp);
                }
            }
            // leftover
            {
                int rest_size = n % block_size;
                if(rest_size == 0) rest_size = block_size;
                memset(int_data_buffer.data(), 0, rest_size * sizeof(T_fp));
                decode_block(streams_pos, rest_size, num_bitplanes, int_data_buffer.data());
                for(int j=0; j<rest_size; j++){
                    *(data_pos++) = ldexp((T_data) negabinary2binary(int_data_buffer[j]), - ending_bitplane + exp);
                }
            }
            return data;
        }

        void print() const {
            std::cout << "Grouped bitplane encoder" << std::endl;
        }
    private:
        template<class T>
        uint32_t block_size_based_on_bitplane_int_type() const {
            uint32_t block_size = 0;
            if(std::is_same<T, uint64_t>::value){
                block_size = 64;
            }
            else if(std::is_same<T, uint32_t>::value){
                block_size = 32;
            }
            else if(std::is_same<T, uint16_t>::value){
                block_size = 16;
            }
            else if(std::is_same<T, uint8_t>::value){
                block_size = 8;
            }
            else{
                std::cerr << "Integer type not supported." << std::endl;
                exit(0);
            }
            return block_size;
        }
        inline uint64_t binary2negabinary(const int64_t x) const {
            return (x + (uint64_t)0xaaaaaaaaaaaaaaaaull) ^ (uint64_t)0xaaaaaaaaaaaaaaaaull;
        }
        inline uint32_t binary2negabinary(const int32_t x) const {
            return (x + (uint32_t)0xaaaaaaaau) ^ (uint32_t)0xaaaaaaaau;
        }
        inline int64_t negabinary2binary(const uint64_t x) const {
            return (x ^0xaaaaaaaaaaaaaaaaull) - 0xaaaaaaaaaaaaaaaaull;
        }

        inline int32_t negabinary2binary(const uint32_t x) const {
            return (x ^0xaaaaaaaau) - 0xaaaaaaaau;
        }
        template <class T_int>
        inline void encode_block(T_int const * data, size_t n, uint8_t num_bitplanes, std::vector<T_stream *>& streams_pos) const {
            for(int k=num_bitplanes - 1; k>=0; k--){
                T_stream bitplane_value = 0;
                T_stream bitplane_index = num_bitplanes - 1 - k;
                for (int i=0; i<n; i++){
                    bitplane_value += (T_stream)((data[i] >> k) & 1u) << i;
                }
                *(streams_pos[bitplane_index] ++) = bitplane_value;
            }
        }
        template <class T_int>
        inline void decode_block(std::vector<T_stream const *>& streams_pos, size_t n, uint8_t num_bitplanes, T_int * data) const {
            for(int k=num_bitplanes - 1; k>=0; k--){
                T_stream bitplane_index = num_bitplanes - 1 - k;
                T_stream bitplane_value = *(streams_pos[bitplane_index] ++);
                for (int i=0; i<n; i++){
                    data[i] += ((bitplane_value >> i) & 1u) << k;
                }
            }
        }
    };
}
#endif
