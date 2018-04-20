#include <cstddef>
#include <utility>

namespace folly {
	template <typename T>
	using _t = typename T::type;

	namespace detail {

		/**
		* Given a metafunction class `Fn` and arguments `Ts...`, invoke `Fn`
		* with `Ts...`.
		*/
		template <class Fn, class... Ts>
		using MetaApply = typename Fn::template apply<Ts...>;

		/**
		* A list of types.
		*/
		template <class... Ts>
		struct TypeList {
			/**
			* An alias for this list of types
			*/
			using type = TypeList;

			/**
			* \return the number of types in this list.
			*/
			static constexpr std::size_t size() noexcept {
				return sizeof...(Ts);
			}

			/**
			* This list of types is also a metafunction class that accepts another
			* metafunction class and invokes it with all the types in the list.
			*/
			template <class Fn>
			using apply = MetaApply<Fn, Ts...>;
		};

		/**
		* An empty struct.
		*/
		struct Empty {};

		/**
		* Defers the evaluation of an alias.
		*
		* Given a template `C` and arguments `Ts...`, then
		* - If `C<Ts...>` is well-formed, `MetaApply<MetaDefer<C, Ts...>>` is well-
		*   formed and is an alias for `C<Ts...>`.
		* - Otherwise, `MetaApply<MetaDefer<C, Ts...>>` is ill-formed.
		*/

		/// \cond
		namespace impl {
			template <bool B>
			struct If_ {
				template <class T, class U>
				using apply = T;
			};
			template <>
			struct If_<false> {
				template <class T, class U>
				using apply = U;
			};
		} // namespace impl
		  /// \endcond

		  /**
		  * Like std::conditional, but with fewer template instantiations
		  */
		template <bool If_, class Then, class Else>
		using If = MetaApply<impl::If_<If_>, Then, Else>;

		/**
		* Defers the evaluation of an alias.
		*
		* Given a template `C` and arguments `Ts...`, then
		* - If `C<Ts...>` is well-formed, `MetaApply<MetaDefer<C, Ts...>>` is well-
		*   formed and is an alias for `C<Ts...>`.
		* - Otherwise, `MetaApply<MetaDefer<C, Ts...>>` is ill-formed.
		*/
		template <template <class...> class C, class... Ts>
		class MetaDefer {
			template <template <class...> class D = C, class = D<Ts...>>
			static char(&try_(int))[1];
			static char(&try_(long))[2];
			struct Result {
				using type = C<Ts...>;
			};

		public:
			template <class... Us>
			using apply = _t<If<sizeof(try_(0)) - 1 || sizeof...(Us), Empty, Result>>;
		};

		/**
		* A metafunction class that always returns its argument unmodified.
		*
		* `MetaApply<MetaIdentity, int>` is equivalent to `int`.
		*/
		struct MetaIdentity {
			template <class T>
			using apply = T;
		};

		/**
		* Lifts a class template or an alias template to be a metafunction class.
		*
		* `MetaApply<MetaQuote<C>, Ts...>` is equivalent to `C<Ts...>`.
		*/
		template <template <class...> class C>
		struct MetaQuote {
			template <class... Ts>
			using apply = MetaApply<MetaDefer<C, Ts...>>;
		};

		/// \cond
		// Specialization for TypeList since it doesn't need to go through MetaDefer
		template <>
		struct MetaQuote<TypeList> {
			template <class... Ts>
			using apply = TypeList<Ts...>;
		};
		/// \endcond

		/**
		* Given a metafunction class `Fn` and a `TypeList`, call `Fn` with the types
		* in the `TypeList`.
		*/
		/**
		* Given a binary metafunction class, convert it to another binary metafunction
		* class with the argument order reversed.
		*/
		template <class Fn>
		struct MetaFlip {
			template <class A, class B>
			using apply = MetaApply<Fn, B, A>;
		};

		/// \cond
		namespace impl {
			template <class Fn>
			struct FoldL_ {
				template <class... Ts>
				struct Lambda : MetaIdentity {};
				template <class A, class... Ts>
				struct Lambda<A, Ts...> {
					template <class State>
					using apply = MetaApply<Lambda<Ts...>, MetaApply<Fn, State, A>>;
				};
				template <class A, class B, class C, class D, class... Ts>
				struct Lambda<A, B, C, D, Ts...> { // manually unroll 4 elements
					template <class State>
					using apply = MetaApply<
						Lambda<Ts...>,
						MetaApply<
						Fn,
						MetaApply<Fn, MetaApply<Fn, MetaApply<Fn, State, A>, B>, C>,
						D>>;
				};
				template <class... Ts>
				using apply = Lambda<Ts...>;
			};
		} // namespace impl
		  /// \endcond

		  /**
		  * Given a `TypeList`, an initial state, and a binary function, reduce the
		  * `TypeList` by applying the function to each element and the current state,
		  * producing a new state to be used with the next element. This is a "left"
		  * fold, in functional parlance.
		  *
		  * `TypeReverseFold<TypeList<A, B, C>, X, Fn>` is equivalent to
		  * `MetaApply<Fn, MetaApply<Fn, MetaApply<Fn, X, C>, B, A>`.
		  */
		template <class List, class State, class Fn>
		using TypeReverseFold = MetaApply<MetaApply<List, impl::FoldL_<Fn>>, State>;

		
	} // namespace detail
} // namespace folly

using namespace folly;
using namespace folly::detail;


using Nil = Empty;
template <class Car, class Cdr = Nil>
struct Cons {};
using Fn = MetaQuote<Cons>;
using T4 = TypeReverseFold<
	TypeList<int, short, void, int*, short*, void*>,
	Nil,
	MetaFlip<Fn>>;
using E4 = Cons<
	void*,
	Cons<short*, Cons<int*, Cons<void, Cons<short, Cons<int, Nil>>>>>>;

int main()
{
	static_assert(std::is_same<T4, E4>::value, "");
    return 0;
}