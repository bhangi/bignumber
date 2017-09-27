#ifndef BIG_INT_HPP
#define BIG_INT_HPP

#include <vector>
#include <algorithm>
#include <string>
#include <iterator>
#include <type_traits>
#include <sstream>
#include <exception>
#include <future>
#include <cstdint>
#include <numeric>

namespace mystl {
// in-order transform
// (Note: std::transform doesn't guarantee order of functors)
template<class InputIt1, class InputIt2,
         class OutputIt, class BinaryOperation>
OutputIt transform(InputIt1 first1, InputIt1 last1, InputIt2 first2,
                   OutputIt d_first, BinaryOperation binary_op)
{
    while (first1 != last1) {
        *d_first++ = binary_op(*first1++, *first2++);
    }
    return d_first;
}

template<class InputIt, class OutputIt, class UnaryOperation>
OutputIt transform(InputIt first1, InputIt last1, OutputIt d_first,
                   UnaryOperation unary_op)
{
    while (first1 != last1) {
        *d_first++ = unary_op(*first1++);
    }
    return d_first;
}

} // namespace mystl


template<typename T>
class bignum_traits {
public:
    typedef T digit_type;

    static constexpr bool lt(const T a, const T b) { // a < b
    	return a < b;
    }
    static constexpr T sum(const T a, const T b) { // a+b
        return a + b;
    }
    static constexpr T sub(const T a, const T b) {   // a-b
        return a - b;
    }
    static constexpr T mul(const T a, const T b) {   // a*b
        return a * b;
    }

    template<typename input>
    static constexpr T input_converter(input&& x) {
    	static_assert((std::is_same<std::decay_t<input>, char>{}
    	 || std::is_same<std::decay_t<input>, signed char>{}), "invalid input type");
      if (x >= '0' && x <= '9')
      	return static_cast<T>(x-char('0'));
      else
        throw std::logic_error("invalid input char");
    }

    template<typename input, typename output>
    static constexpr auto output_converter(input&& x, output&& offset) -> decltype(x+offset) {
    	static_assert((std::is_same<std::decay_t<output>, char>{}
    			&& std::is_integral<std::decay_t<T>>{}), "invalid output type");
    		return static_cast<std::decay_t<output>>(x+offset);
    }
};

// storage class of digits,
// provides interfaces to manipulate underlying digits data
// Note: digits will be stored least to most significant digits
template<typename T, typename bignum_traits = bignum_traits<T>>
class bignum_storage {
  template<typename Iter>
  using IsRandomIterator = std::is_base_of<std::random_access_iterator_tag,
                      typename std::iterator_traits<Iter>::iterator_category>;

    typedef T digit_type;
    typedef std::vector<T> container_type;

    // internal usage constructor
    bignum_storage(container_type&& rhs) : m_blk{std::move(rhs)} { prune_zeros(); }

public:
    typedef bignum_traits  traits_type;
    typedef typename container_type::iterator iterator;
    typedef typename container_type::const_iterator const_iterator;

    bignum_storage() : m_blk{} {m_blk.push_back(digit_type(0));}
    // SFINAE: with sfinae, overload resolution for function can be done by (return, param list or arg)
    // with constructors its different because they can't return and no two constructors can have
    // same param list. 
    template<typename Iter>
    explicit bignum_storage(Iter b, typename std::enable_if<!IsRandomIterator<Iter>::value, Iter>::type e) {
      //std::cerr << "non random access constructor" << std::endl;
    	std::transform(b, e, std::back_inserter(m_blk),	[](auto&& x) {
    				return traits_type::input_converter(std::forward<decltype(x)>(x));
    			});
      prune_zeros();
    }

    template<typename Iter>
    explicit bignum_storage(Iter b, typename std::enable_if<IsRandomIterator<Iter>::value, Iter>::type e) {
      //std::cerr << "random access constructor" << std::endl;
    	m_blk.reserve(std::distance(b, e));
    	std::transform(b, e, std::back_inserter(m_blk),	[](auto&& x) {
    				return traits_type::input_converter(std::forward<decltype(x)>(x));
    			});
      prune_zeros();
    }

    inline std::size_t size() { return m_blk.size(); }
    inline std::size_t size() const { return m_blk.size(); }

    inline iterator begin(){ return iterator(m_blk.begin()); }
    inline iterator end()  { return iterator(m_blk.end()); }

    inline const_iterator begin() const  { return const_iterator(m_blk.cbegin()); }
    inline const_iterator cbegin() const { return const_iterator(m_blk.cbegin()); }
    inline const_iterator end() const    { return const_iterator(m_blk.cend()); }
    inline const_iterator cend() const   { return const_iterator(m_blk.cend()); }

    inline auto rbegin() { return m_blk.rbegin(); }
    inline auto rend()   { return m_blk.rend(); }
    inline auto crbegin() const { return m_blk.crbegin(); }
    inline auto crend() const   { return m_blk.crend(); }
    inline auto rbegin() const  { return m_blk.crbegin(); }
    inline auto rend() const    { return m_blk.crend(); }

    // remove trailing zeros
    bignum_storage& prune_zeros() {
    	// 300210000 100
    	auto x = std::find_if_not(m_blk.rbegin(), m_blk.rend(), [](auto x) {
    		return x == digit_type(0);
    	});
    	auto last = std::next(m_blk.begin(),std::distance(x, m_blk.rend()));
    	m_blk.erase(last, m_blk.end());
      if (m_blk.empty()) m_blk.push_back(digit_type(0));
      return *this;
    }

    inline bignum_storage& prepend(const digit_type digit, size_t n = 1) {
        m_blk.reserve(size()+n);
        std::fill_n(std::back_inserter(m_blk), n, digit);
        return (*this);
    }

    inline bignum_storage& push_back(const digit_type digit) {
      m_blk.push_back(digit);
      return (*this);
    }
    inline bignum_storage& append(const digit_type digit, size_t n = 1) {
      container_type tmp;
      tmp.reserve(n + m_blk.size());
      std::fill_n(std::back_inserter(tmp), n, digit);
      std::copy(m_blk.begin(), m_blk.end(), std::back_inserter(tmp));
      std::swap(m_blk, tmp);
      prune_zeros();
      return *this;
    }

    inline size_t resize(size_t __new_size, const digit_type __digit=digit_type(0)) {
        m_blk.resize(__new_size, __digit);
        return m_blk.size();
    }

    auto split(size_t at) const {
      if (at >= 0 && at < size())
        return std::make_pair(bignum_storage(container_type(begin(), std::next(begin(), at))),
                bignum_storage(container_type(std::next(begin(), at), end())));
      else {
        return std::make_pair(bignum_storage(*this), bignum_storage());
      }
    }

protected:
    container_type m_blk;
};

// digit adder
template<typename T>
class digit_adder {
  const int base;
	T carry;
public:
    explicit constexpr digit_adder(int base) : base(base), carry(0) {}
    constexpr T operator()(const T a, const T b) {
      auto sum = bignum_traits<T>::sum(carry, bignum_traits<T>::sum(a, b));
      if (sum < base) {
        carry = 0;
      } else {
        carry = 1;
        sum -= base;
      }
      return sum;
    }
    bool has_carry() { return carry == 1; }
};

// digit subtractor
template<typename T>
class digit_subtractor {
  const int base;
	T carry;
public:
    explicit constexpr digit_subtractor(int base) : base(base), carry(0) {}
    constexpr T operator()(const T a, const T b) { // note: b-a
      auto x = bignum_traits<T>::sum(b, carry);
      if (bignum_traits<T>::lt(x, a)) { // a < b
    	  carry = -1;
    	  return bignum_traits<T>::sub(bignum_traits<T>::sum(base, x), a);
      } else {
    	  carry = 0;
    	  return bignum_traits<T>::sub(x, a);
      }
    }
    bool has_carry() { return carry == -1; }
};

template<typename T>
struct digit_multiplier {
  const int base;
	T carry, d1;

    explicit constexpr digit_multiplier(int base, T d) : base(base), carry(0), d1(d) {}
    constexpr T operator()(const T a, const T b) {
      auto x = bignum_traits<T>::sum(carry, bignum_traits<T>::mul(a, b));
      carry = x/base;
      return x%base;
    }
    bool has_carry() { return carry != 0; }
};


class big_unsigned;
big_unsigned operator + (const big_unsigned& a, const big_unsigned& b);
big_unsigned operator - (const big_unsigned& a, const big_unsigned& b);
big_unsigned karatsuba_mul(const big_unsigned& n1, const big_unsigned& n2);


// big unsigned class
class big_unsigned {
public:
    // define the big unsigned traits
	typedef bignum_traits<int8_t> bigunsigned_traits;
	using digit_type = typename bigunsigned_traits::digit_type;
  using storage_type = bignum_storage<digit_type, bigunsigned_traits>;

private:
	int base; // base of number
	bignum_storage<digit_type, bigunsigned_traits> digits; // digits

  // private convenient constructor
  big_unsigned(storage_type&& d): base{10}, digits{std::move(d)} {
    //std::cerr << "private move big_unsigned" << std::endl;
  }
public:
    // constructors
    explicit big_unsigned() : big_unsigned(0) {}

    explicit big_unsigned(const std::string& rhs) : base(10), digits(rhs.rbegin(), rhs.rend()) {
    //  std::cerr << "lvalue string constructor" << std::endl;
    }
    explicit big_unsigned(std::string&& rhs) : base(10), digits(rhs.rbegin(), rhs.rend()) {
    //  std::cerr << "rvalue string constructor" << std::endl;
    }

    // integral type
    template<typename IntType, typename = std::enable_if_t<std::is_integral<IntType>::value>>
    explicit big_unsigned(IntType&& rhs)
            : big_unsigned(std::to_string(std::forward<IntType>(rhs))) {
    //  std::cerr << "integral constructor" << "(" << rhs << ")" << std::endl;
    }

    big_unsigned(const big_unsigned&) = default;
    big_unsigned& operator = (const big_unsigned&) = default;
    big_unsigned(big_unsigned&&) = default;
    big_unsigned& operator = (big_unsigned&&) = default;

    // from iterator
    template<typename Iter>
    explicit big_unsigned(Iter b, Iter e) : base(10), digits(b, e) {}

    inline std::size_t size()       { return digits.size(); }
    inline std::size_t size() const { return digits.size(); }

    // observers
    inline auto begin(){ return digits.cbegin(); }
    inline auto end()  { return digits.cend(); }

    inline auto begin() const { return digits.cbegin(); }
    inline auto end() const   { return digits.cend(); }


    big_unsigned& operator += (const big_unsigned& rhs) {
      // TODO: check base is same ?
    	auto newsize = std::max(size(), rhs.size());
    	digits.resize(newsize);
    	digit_adder<digit_type> adder(base);
    	// note: function objects may be copied and then used in algo,
    	// so use std::ref to refer to stateful functors
    	auto it = mystl::transform(rhs.begin(), rhs.end(), digits.cbegin(),
    			               digits.begin(), std::ref(adder));
    	it = mystl::transform(it, digits.end(), it, [&adder](auto&& x) { return adder(x, 0);});
    	if (adder.has_carry()) digits.prepend(1);
      return (*this);
    }

    big_unsigned& operator -= (const big_unsigned& rhs) {
      if ((*this) < rhs) {
          std::stringstream ss;
          ss << "size: " << size() << " < " << rhs.size() << ", negative";
          throw std::logic_error(ss.str());
      }

      digit_subtractor<digit_type> f(base);
      auto it = mystl::transform(rhs.begin(), rhs.end(),
      		digits.cbegin(), digits.begin(), std::ref(f));
      mystl::transform(it, digits.end(), it, [&f](const auto x){return f(0, x);});
      digits.prune_zeros();
      return *this;
    }

    std::pair<big_unsigned, big_unsigned> split(size_t at) const {
      if (at < size()) {
          auto x = digits.split(size()-at);
          return std::make_pair(big_unsigned(std::move(x.second)), big_unsigned(std::move(x.first))); 
      }
      return std::make_pair(big_unsigned(*this), big_unsigned(0));
    }

    big_unsigned& operator *= (const big_unsigned& rhs) {
      big_unsigned tmp = karatsuba_mul(*this, rhs);
      *this = std::move(tmp);
      return *this;
    }

protected:

    inline std::uint64_t to_ullong() const { // cast operators danger
      static const std::uint64_t table[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};
      //return std::transform_reduce(n1.begin(), n1.end(), std::cbegin(table), 0ULL);
      return std::inner_product(begin(), end(), std::cbegin(table), 0ULL);
    }

    friend big_unsigned classic_mul(const big_unsigned& n1, const big_unsigned& n2) {
      big_unsigned ret(0);
      int i = 0;
      std::for_each(n2.begin(), n2.end(), [&n1, &ret, &i, &n2](auto&& d2) {
          storage_type tmp; // 316*5291
          tmp.resize(i + std::max(n1.size(), n2.size()));
          auto it = std::next(tmp.begin(), i);
          digit_multiplier<big_unsigned::digit_type> f(n1.base, 0);

          std::for_each(n1.begin(), n1.end(), [&it,&f,&d2](auto&& d1) {
              *it++ = f(d1, d2);
          });
          if (f.has_carry()) tmp.push_back(f.carry);
//          std::for_each(tmp.begin(), tmp.end(), [](auto&& c) { std::cout << char('0'+c) << ", "; });
          tmp.prune_zeros();
//          big_unsigned x(std::move(tmp));
//          std::cout << "x = " << x << std::endl;

          ret += big_unsigned(std::move(tmp));
          ++i;
        });
//      std::cout << n1.size() << ", " << n2.size() << std::endl;
      return ret;
    }


    friend big_unsigned karatsuba_mul(const big_unsigned& n1, const big_unsigned& n2) {
    	auto m = std::max(n1.size(), n2.size());
    	auto n = std::min(n1.size(), n2.size());

      using namespace std::chrono_literals;
      if (m <= 9) {
    	  std::uint64_t x = n1.to_ullong(), y = n2.to_ullong();
        //std::cout << x << " * " << y << " == " << x*y << std::endl;
        return big_unsigned(x*y);
    	}
      else if((m <= 8192) && (n > 0) && (n <= 5)) {
        auto base_mul_fn = [](const big_unsigned& a, const big_unsigned& b) {
            int n = a.to_ullong();
            if (n == 0) return big_unsigned(0);
            big_unsigned tmp(b);
            while(--n > 0) tmp += b;
            return tmp;
        };
        return (n1.size() < n2.size()) ? base_mul_fn(n1, n2) : base_mul_fn(n2, n1);
      }
      else if ((n > 9) && (m > 8192)) {
          auto l1h1(n1.digits.split(m/2)), l2h2(n2.digits.split(m/2));
          const big_unsigned low1(std::move(l1h1.first)), high1(std::move(l1h1.second)),
                     low2(std::move(l2h2.first)), high2(std::move(l2h2.second));
        
          // parallelize the calls
          auto fz0 = std::async(karatsuba_mul, std::cref(low1), std::cref(low2));
          auto fz2 = std::async(karatsuba_mul, std::cref(high1), std::cref(high2));

          auto fs1 = std::async(operator+, std::cref(low1), std::cref(high1));
          auto fs2 = std::async(operator+, std::cref(low2), std::cref(high2));
          auto fz1 = std::async(karatsuba_mul, fs1.get(), fs2.get());

          big_unsigned z2 = fz2.get();
          big_unsigned z0 = fz0.get();

          big_unsigned z3 = fz1.get() - (z2 + z0);
          z3.digits.append(digit_type(0), (m/2));
          z2.digits.append(digit_type(0), 2*(m/2));
//        std::cout << "split: {{z0 = " << z0 << ", " << z2 << ", " << z3 << "}}" << std::endl;

          z2 += z3;
          z2 += z0;
          return z2;
      }
      else {
          auto l1h1(n1.digits.split(m/2)), l2h2(n2.digits.split(m/2));
          const big_unsigned low1(std::move(l1h1.first)), high1(std::move(l1h1.second)),
                     low2(std::move(l2h2.first)), high2(std::move(l2h2.second));
        
      		big_unsigned z0 = karatsuba_mul(low1, low2);
          big_unsigned z1 = karatsuba_mul(low1+high1, low2+high2);
          big_unsigned z2 = karatsuba_mul(high1, high2);
          big_unsigned z3 = z1-(z2+z0);

          z3.digits.append(digit_type(0), (m/2));
          z2.digits.append(digit_type(0), 2*(m/2));
//        std::cout << "split: {{z0 = " << z0 << ", " << z2 << ", " << z3 << "}}" << std::endl;

          z2 += z3;
          z2 += z0;
//        std::cout << n1 << "*" << n2  << "==" << z2 << std::endl;
          //if (m <= 100) assert(z2 == classic_mul(n1, n2));
          return z2;
    	}
    }
public:

    // observers
    bool operator == (const big_unsigned& rhs) const {
        return rhs.size() == size()
        		&& std::equal(digits.begin(), digits.end(), rhs.digits.begin());
    }

    bool operator != (const big_unsigned& rhs) const {
        return !(operator == (rhs));
    }

    bool operator < (const big_unsigned& rhs) const {
        size_t rhs_size = rhs.size();
        if (size() < rhs_size) return true;
        else if (size() > rhs_size) return false;
        else return std::lexicographical_compare(digits.rbegin(), digits.rend(),
        		                        rhs.digits.rbegin(), rhs.digits.rend());
    }

    big_unsigned& operator++() {
      operator+= (big_unsigned(1));
      return *this;
    }


    big_unsigned operator++(int) {
      big_unsigned tmp(*this);
      ++(*this);
      return tmp;
    }
    
    friend std::ostream& operator << (std::ostream& oss, const big_unsigned& rhs) {
      const std::ostream::char_type zero('0');
      mystl::transform(rhs.digits.crbegin(), rhs.digits.crend(), std::ostream_iterator<char>(oss), [&zero](auto&& x) {
    	  return bigunsigned_traits::output_converter(x, zero);
    	});
      return oss;
    }

    friend std::istream& operator >> (std::istream& iss, big_unsigned& rhs) {
        std::string _in;
        iss >> _in;
        rhs = big_unsigned(_in);

        return iss;
    }
};

 

big_unsigned operator + (const big_unsigned& a, const big_unsigned& b) {
		big_unsigned tmp(a);
		tmp += b;
		return tmp;
}

big_unsigned operator - (const big_unsigned& a, const big_unsigned& b) {
		big_unsigned tmp(a);
		tmp -= b;
		return tmp;
}

big_unsigned operator * (const big_unsigned& a, const big_unsigned& b) {
		big_unsigned tmp(a);
		tmp *= b;
		return tmp;
}


#endif // BIG_INT_HPP
