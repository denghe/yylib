#pragma once
#include "yy_ptr.h"
#include "yy_buffer.h"
#include "yy_string.h"

// 辅助宏在最下面

namespace yy {

	/************************************************************************************/
	// for object 序列化等需求

	// type_id 映射
	template<typename T>
	struct type_id {
		static const uint16_t value = 0;
	};

	template<typename T>
	constexpr uint16_t type_id_v = type_id<T>::value;

	/*
	// 方便复制

namespace xx {
	template<>
	struct type_id<XXXXXXXXXXXXXX> {
		static const uint16_t value = XXXXXXXX;
	};
}
*/

	// 扩展智能指针的头部, 直接包含 type id 以及 序列化过程中要使用到的 offset 变量占位
	struct shared_ptr_object_header : shared_ptr_header {
		union {
			struct {
				uint32_t typeId;        // 序列化 或 类型转换用
				uint32_t offset;        // 序列化等过程中使用
			};
			void* ud;
		};

		template<typename T>
		void init() {
			this->shared_ptr_header::init<T>();
			typeId = type_id_v<T>;
			offset = 0;
		}
	};

	struct object;

	template<typename T>
	struct shared_ptr_header_switcher<T, std::enable_if_t< (std::is_same_v<object, T> || type_id_v<T> > 0) >> {
		using type = shared_ptr_object_header;
	};


	using object_s = shared_ptr<object>;
	struct object_handler;

	/************************************************************************************/
	// 接口函数适配模板. 特化 以扩展类型支持
	template<typename T, typename ENABLED = void>
	struct object_interface {
		static inline void Write(object_handler& om, Data& d, T const& in) {
			std::string s(TypeName_v<T>);
			assert(false);
		}
		static inline void WriteFast(object_handler& om, Data& d, T const& in) {
			std::string s(TypeName_v<T>);
			assert(false);
		}
		static inline int Read(object_handler& om, Data_r& d, T& out) {
			return 0;
		}
		static inline void Append(object_handler& om, std::string& s, T const& in) {
			return yy::Append(s, in);
		}
		static inline void AppendCore(object_handler& om, std::string& s, T const& in) {
			std::string ss(TypeName_v<T>);
			assert(false);
		}
		static inline void Clone(object_handler& om, T const& in, T& out) {
			out = in;
		}
		static inline int RecursiveCheck(object_handler& om, T const& in) {
			return 0;
		}
		static inline void RecursiveReset(object_handler& om, T& in) {
		}
		static inline void SetDefaultValue(object_handler& om, T& in) {
		}
	};


	/************************************************************************************/
	// 方便复制
	/*
	inline void Write(yy::object_handler& o, yy::Data& d) const override { }
	inline void WriteFast(yy::object_handler& o, yy::Data& d) const override { }
	inline int Read(yy::object_handler& o, yy::Data_r& d) override { }
	inline void Append(yy::object_handler& o) const override { }
	inline void AppendCore(yy::object_handler& o) const override { }
	inline void Clone(yy::object_handler& o, void* const& tar) const override { }
	inline int RecursiveCheck(yy::object_handler& o) const override { }
	inline void RecursiveReset(yy::object_handler& o) override { }
	inline void SetDefaultValue(yy::object_handler& o) { }
	*/

	// object: 仅用于 shared_ptr<> weak_ptr<> 包裹的类型基类. 只能单继承. 否则影响父子判断
	struct object {
		// 派生类须加这个声明以便于 Register 时探索父子关系
		using BaseType = void;

		// 派生类都需要有默认构造。
		object() = default;

		virtual ~object() = default;

		// 序列化
		virtual void Write(object_handler& om, yy::Data& d) const = 0;

		// 反序列化
		virtual int Read(object_handler& om, yy::Data_r& d) = 0;

		// 输出 json 长相时用于输出外包围 {  } 部分
		virtual void Append(object_handler& om, std::string& s) const = 0;

		// 输出 json 长相时用于输出花括号内部的成员拼接
		virtual void AppendCore(object_handler& om, std::string& s) const = 0;

		// 克隆( 效果类似 Write + Read ), 遇到 引用到 野shared_ptr 的 weak_ptr 将保留原值
		virtual void Clone(object_handler& om, void* const& tar) const = 0;

		// 返回成员变量( 如果是 shared_ptr 的话 )是否存在递归引用
		virtual int RecursiveCheck(object_handler& om) const = 0;

		// 向 o 传递所有 shared_ptr<T> member 以斩断循环引用 防止内存泄露
		virtual void RecursiveReset(object_handler& om) = 0;

		// 恢复成员变量初始值
		virtual void SetDefaultValue(object_handler& om) = 0;

		// 注意: 如果类以值类型方式使用, 则下列函数不可用
		// 注意: 下面两个函数, 不可以在析构函数中使用, 构造函数中使用也需要确保构造过程顺利无异常。另外，如果指定 T, 则 unsafe, 需小心确保 this 真的能转为 T
		// 得到当前类的强指针
		template<typename T = object>
		YY_INLINE shared_ptr<T> SharedFromThis() const {
			auto h = (shared_ptr_object_header*)this - 1;
			return (*((weak_ptr<T>*) & h)).Lock();
		}

		// 得到当前类的弱指针
		template<typename T = object>
		YY_INLINE weak_ptr<T> WeakFromThis() const {
			auto h = (shared_ptr_object_header*)this - 1;
			return *((weak_ptr<T>*) & h);
		}

		// 得到当前类的 typeId( 
		YY_INLINE int16_t GetTypeId() const {
			auto h = (shared_ptr_object_header*)this - 1;
			return (int16_t)h->typeId;
		}
	};


	/************************************************************************************/
	// object 相关操作类. 注册 typeId 与 关联 Create 函数

	template<>
	struct type_id<object> {
		static const uint16_t value = 0;
	};

	YY_HAS_TYPEDEF(IsSimpleType_v);

	struct object_handler {
		// 公共上下文
		std::vector<void*> ptrs;								// for write, append, clone
		std::vector<void*> ptrs2;								// for read, clone
		std::vector<std::pair<shared_ptr_object_header*, shared_ptr_object_header**>> weaks;	// for clone

		inline static object_s null;

		// 类创建函数
		typedef object_s(*FT)();

		// typeId 创建函数 映射容器
		inline static std::array<FT, 65536> fs{};

		// 存储 typeId 的 父typeId 的下标
		inline static std::array<uint16_t, 65536> pids{};

		// 存储 typeId 对应的 Type 是否为 "简单类型"( 只含有基础数据类型, 可跳过递归检测，简化序列化操作 )
		inline static std::array<bool, 65536> simples{};

		// 根据 typeid 判断父子关系
		YY_INLINE static bool IsBaseOf(uint32_t const& baseTypeId, uint32_t typeId) noexcept {
			for (; typeId != baseTypeId; typeId = pids[typeId]) {
				if (!typeId || typeId == pids[typeId]) return false;
			}
			return true;
		}

		// 根据 类型 判断父子关系
		template<typename BT>
		YY_INLINE static bool IsBaseOf(uint32_t const& typeId) noexcept {
			static_assert(std::is_base_of_v<object, BT>);
			return IsBaseOf(type_id_v<BT>, typeId);
		}

		// 根据 类型 判断父子关系
		template<typename BT, typename T>
		YY_INLINE static bool IsBaseOf() noexcept {
			static_assert(std::is_base_of_v<object, T>);
			static_assert(std::is_base_of_v<object, BT>);
			return IsBaseOf(type_id_v<BT>, type_id_v<T>);
		}

		// 避开 dynamic_case 的快速实现
		template<typename T, typename U>
		YY_INLINE static shared_ptr<T>& As(shared_ptr<U> const& v) noexcept {
			static_assert(std::is_base_of_v<object, T>);
			static_assert(std::is_base_of_v<object, U>);
			if constexpr (std::is_same_v<U, T> || std::is_base_of_v<T, U>) {
				return v.template ReinterpretCast<T>();
			}
			else {
				if (!v || !IsBaseOf<T>(v.GetHeader()->typeId)) {
					return null.template ReinterpretCast<T>();
				}
				return v.template ReinterpretCast<T>();
			}
		}

		// 关联 typeId 与创建函数, 顺便再填充点别的
		template<typename T>
		YY_INLINE static void Register() {
			static_assert(std::is_base_of_v<object, T>);
			pids[type_id_v<T>] = type_id_v<typename T::BaseType>;
			fs[type_id_v<T>] = []() -> object_s { return Make<T>(); };
			if constexpr (IsSimpleType_v<T>) {
				if constexpr (std::is_same_v<typename T::IsSimpleType_v, T>) {
					simples[type_id_v<T>] = true;
				}
			}
		}

		// 根据 typeId 来创建对象. 失败返回空
		YY_INLINE static object_s Create(uint16_t const& typeId) {
			if (!typeId || !fs[typeId]) return nullptr;
			return fs[typeId]();
		}

        // 向 data 写入数据( 支持 shared_ptr<T> 或 T 结构体 ). 会初始化写入上下文, 并在写入结束后擦屁股( 主要入口 )
		// 如果 v 是 shared_ptr<T> 类型 且 v 的类型 和 T 完全一致( 并非基类 ), 则可 令 direct = true 以加速写入操作
		// 如果有预分配 data 的内存，可设置 needReserve 为 false. 主要针对结构体嵌套的简单类型. 遇到 "类" 会阻断 ( 需有充分把握，最好在结束后 assert( d.len <= d.cap ) )
		template<bool needReserve = true, bool direct = false, typename T>
		YY_INLINE void WriteTo(Data& d, T const& v) {
			if constexpr (IsShared_v<T>) {
				assert(v);
				using U = typename T::ElementType;
				if constexpr (direct) {
					assert(((shared_ptr_object_header*)v.pointer - 1)->typeId == type_id_v<U>);
				}
				if constexpr (direct && IsSimpleType_v<U>) {
					d.WriteVarInteger<needReserve>(type_id_v<U>);
					v.pointer->U::Write(*this, d);
					return;
				}
				else {
					auto tid = ((shared_ptr_object_header*)v.pointer - 1)->typeId;
					if (simples[tid]) {
						d.WriteVarInteger<needReserve>(tid);
						Write_<needReserve>(d, *v.pointer);
						return;
					}
					else {
						Write_<needReserve, true>(d, v);
					}
				}
			}
			else {
				Write_<needReserve>(d, v);
			}
			if constexpr (!IsSimpleType_v<T>) {
				for (auto&& p : ptrs) {
					*(uint32_t*)p = 0;
				}
				ptrs.clear();
			}
		}

        // before WriteTo, d.Clear()
        template<bool needReserve = true, bool direct = false, typename T>
        YY_INLINE void ClearAndWriteTo(Data& d, T const& v) {
            d.Clear();
            WriteTo<needReserve, direct, T>(d, v);
		}

    protected:
		// 内部函数
		template<bool needReserve = true, bool isFirst = false, typename T>
		YY_INLINE void Write_(Data& d, T const& v) {
			if constexpr (IsShared_v<T>) {
				using U = typename T::ElementType;
				if constexpr (std::is_base_of_v<object, U>) {
				    static_assert(std::is_same_v<object, U> || type_id_v<U> > 0);
					if (!v) {
						// 如果是 空指针， offset 值写 0
						d.WriteFixed<needReserve>((uint8_t)0);
					}
					else {
						// 写入格式： idx + typeId + content ( idx 临时存入 h->offset )
						auto h = ((shared_ptr_object_header*)v.pointer - 1);
						if (h->offset == 0) {
							ptrs.push_back(&h->offset);
							h->offset = (uint32_t)ptrs.size();
							if constexpr (!isFirst) {
								d.WriteVarInteger<needReserve>(h->offset);
							}
							d.WriteVarInteger<needReserve>(h->typeId);
							Write_<needReserve>(d, *v.pointer);
						}
						else {
							d.WriteVarInteger<needReserve>(h->offset);
						}
					}
				}
				else {
					if (v) {
						d.WriteFixed<needReserve>((uint8_t)1);
						Write_<needReserve>(d, *v);
					}
					else {
						d.WriteFixed<needReserve>((uint8_t)0);
					}
				}
			}
			else if constexpr (IsWeak_v<T>) {
				if (v) {
					auto p = v.h + 1;
					Write_<needReserve>(d, *(shared_ptr<typename T::ElementType>*) & p);
				}
				else {
					d.WriteFixed<needReserve>((uint8_t)0);
				}
			}
			else if constexpr (std::is_base_of_v<object, T>) {
				v.Write(*this, d);
			}
			else if constexpr (IsOptional_v<T>) {
				if (v.has_value()) {
					d.WriteFixed<needReserve>((uint8_t)1);
					Write_<needReserve>(d, *v);
				}
				else {
					d.WriteFixed<needReserve>((uint8_t)0);
				}
			}
			else if constexpr (IsVector_v<T> || IsSetSeries_v<T> || IsQueueSeries_v<T>) {
				d.WriteVarInteger<needReserve>(v.size());
				if (v.empty()) return;
				if constexpr (IsVector_v < T> && (sizeof(T) == 1 || std::is_floating_point_v<T>)) {
					d.WriteFixedArray<needReserve>(v.data(), v.size());
				}
				else if constexpr (std::is_integral_v<typename T::value_type>) {
					if constexpr (needReserve) {
						auto cap = v.size() * (sizeof(T) + 1);
						if (d.cap < cap) {
							d.Reserve<false>(cap);
						}
					}
					for (auto&& o : v) {
						d.WriteVarInteger<false>(o);
					}
				}
				else {
					for (auto&& o : v) {
						Write_<needReserve>(d, o);
					}
				}
			}
			else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>) {
				d.WriteVarInteger<needReserve>(v.size());
				d.WriteBuf<needReserve>(v.data(), v.size());
			}
			else if constexpr (std::is_base_of_v<Span, T>) {
				d.WriteVarInteger<needReserve>(v.len);
				d.WriteBuf<needReserve>(v.buf, v.len);
			}
			else if constexpr (std::is_integral_v<T>) {
				if constexpr (sizeof(T) == 1) {
					d.WriteFixed<needReserve>(v);
				}
				else {
					d.WriteVarInteger<needReserve>(v);
				}
			}
			else if constexpr (std::is_enum_v<T>) {
				Write_<needReserve>(d, *(std::underlying_type_t<T>*) & v);
			}
			else if constexpr (std::is_floating_point_v<T>) {
				d.WriteFixed<needReserve>(v);
			}
			else if constexpr (IsTuple_v<T>) {
				std::apply([&](auto const &... args) {
					(Write_<needReserve>(d, args), ...);
					}, v);
			}
			else if constexpr (IsPair_v<T>) {
				Write<needReserve>(v.first, v.second);
			}
			else if constexpr (IsMapSeries_v<T>) {
				d.WriteVarInteger<needReserve>(v.size());
				for (auto&& kv : v) {
					Write<needReserve>(d, kv.first, kv.second);
				}
			}
			else {
				if constexpr (needReserve) {
					object_interface<T>::Write(*this, d, v);
				}
				else {
					object_interface<T>::WriteFast(*this, d, v);
				}
			}
		}

	public:
		// 转发到 Write_
		template<bool needReserve = true, typename...Args>
		YY_INLINE void Write(Data& d, Args const&...args) {
			static_assert(sizeof...(args) > 0);
			(Write_<needReserve>(d, args), ...);
		}

		// 从 data 读入 / 反序列化, 填充到 v. ( 支持 shared_ptr<T> 或 T 结构体 )( 主要入口 )
		// 原则: 尽量值覆盖, 不新建对象
		template<typename T>
		YY_INLINE int ReadFrom(Data_r& d, T& v) {
			auto r = Read_<T, IsShared_v<T>>(d, v);
			if constexpr (!IsSimpleType_v<T>) {
				ptrs.clear();
				for (auto& p : ptrs2) {
					object_s o;
					o.pointer = (object*)p;
				}
				ptrs2.clear();
			}
			return r;
		}

	protected:
		template<std::size_t I = 0, typename... Tp>
		YY_INLINE std::enable_if_t<I == sizeof...(Tp) - 1, int> ReadTuple(Data_r& d, std::tuple<Tp...>& t) {
			return Read_(d, std::get<I>(t));
		}

		template<std::size_t I = 0, typename... Tp>
		YY_INLINE std::enable_if_t < I < sizeof...(Tp) - 1, int> ReadTuple(Data_r& d, std::tuple<Tp...>& t) {
			if (int r = Read_(d, std::get<I>(t))) return r;
			return ReadTuple<I + 1, Tp...>(d, t);
		}

		// 内部函数
		template<typename T, bool isFirst = false>
		YY_INLINE int Read_(Data_r& d, T& v) {
			if constexpr (IsShared_v<T>) {
				using U = typename T::ElementType;
				if constexpr (std::is_base_of_v<object, U>) {
                    static_assert(std::is_same_v<object, U> || type_id_v<U> > 0);
					uint32_t idx;
					if constexpr (isFirst) {
						idx = 1;
					}
					else {
						if (int r = Read_(d, idx)) return r;
						if (!idx) {
							v.Reset();
							return 0;
						}
					}

					auto len = (uint32_t)ptrs.size();
					if (idx == len + 1) {
						uint16_t typeId;
						if (int r = Read_(d, typeId)) return r;
						if (!typeId) return __LINE__;
                        if (!fs[typeId]) return __LINE__;
						if (!IsBaseOf<U>(typeId)) return __LINE__;

						if (!v || v.typeId() != typeId) {
							v = std::move(Create(typeId).template ReinterpretCast<U>());
							assert(v);
						}
						ptrs.emplace_back(v.pointer);
						if (int r = Read_(d, *v)) return r;
					}
					else {
						if (idx > len) return __LINE__;
						auto& o = *(object_s*)&ptrs[idx - 1];
						if (!IsBaseOf<U>(o.GetHeader()->typeId)) return __LINE__;
						v = o.template ReinterpretCast<U>();
					}
					return 0;
				}
				else {
					uint8_t hasValue;
					if (int r = Read_(d, hasValue)) return r;
					if (!hasValue) {
						v.Reset();
						return 0;
					}
					if (v.Empty()) {
						v = Make<U>();
					}
					return Read_(d, v.Value());
				}
			}
			else if constexpr (IsWeak_v<T>) {
				shared_ptr<typename T::ElementType> o;
				if (int r = Read_(d, o)) {
					KillRecursive(o);
					return r;
				}
				v = o;
				if (o) {
					ptrs2.emplace_back(o.pointer);
					o.pointer = nullptr;
				}
				return 0;
			}
			else if constexpr (std::is_base_of_v<object, T>) {
				return v.Read(*this, d);
			}
			else if constexpr (IsOptional_v<T>) {
				uint8_t hasValue;
				if (int r = Read_(d, hasValue)) return r;
				if (!hasValue) {
					v.reset();
					return 0;
				}
				if (!v.has_value()) {
					v.emplace();
				}
				return Read_(d, v.value());
			}
			else if constexpr (IsVector_v<T>) {
				size_t siz = 0;
				if (int r = Read_(d, siz)) return r;
				if (d.offset + siz > d.len) return __LINE__;
				v.resize(siz);
				if (siz == 0) return 0;
				auto buf = v.data();
				if constexpr (sizeof(T) == 1 || std::is_floating_point_v<T>) {
					d.ReadFixedArray(buf, siz);
				}
				else {
					for (size_t i = 0; i < siz; ++i) {
						if (int r = Read_(d, buf[i])) return r;
					}
				}
				return 0;
			}
			else if constexpr (IsSetSeries_v<T> || IsQueueSeries_v<T>) {
				size_t siz = 0;
				if (int r = Read_(d, siz)) return r;
				if (d.offset + siz > d.len) return __LINE__;
				v.clear();
				if (siz == 0) return 0;
				typename T::value_type o;
				for (size_t i = 0; i < siz; ++i) {
					if (int r = Read_(d, o)) return r;
					if constexpr (IsDeque_v<T>) {
						v.push_back(std::move(o));
					}
					else {
						v.insert(std::move(o));
					}
				}
				return 0;
			}
			else if constexpr (std::is_same_v<T, std::string>) {
				size_t siz;
				if (int r = Read_(d, siz)) return r;
				if (d.offset + siz > d.len) return __LINE__;
				v.assign((char*)d.buf + d.offset, siz);
				d.offset += siz;
				return 0;
			}
			else if constexpr (std::is_same_v<T, Data>) {
				size_t siz;
				if (int r = Read_(d, siz)) return r;
				if (d.offset + siz > d.len) return __LINE__;
				v.Clear();
				v.WriteBuf(d.buf + d.offset, siz);
				d.offset += siz;
				return 0;
			}
			else if constexpr (std::is_integral_v<T>) {
				if constexpr (sizeof(T) == 1) {
					if (int r = d.ReadFixed(v))  return __LINE__ * 1000000 + r;
				}
				else {
					if (int r = d.ReadVarInteger(v)) return __LINE__ * 1000000 + r;
				}
				return 0;
			}
			else if constexpr (std::is_enum_v<T>) {
				return Read_(d, *(std::underlying_type_t<T>*) & v);
			}
			else if constexpr (std::is_floating_point_v<T>) {
				if (int r = d.ReadFixed(v)) return __LINE__ * 1000000 + r;
				return 0;
			}
			else if constexpr (IsTuple_v<T>) {
				return ReadTuple(v);
			}
			else if constexpr (IsPair_v<T>) {
				return Read(v.first, v.second);
			}
			else if constexpr (IsMapSeries_v<T>) {
				size_t siz;
				if (int r = Read_(d, siz)) return r;
				if (siz == 0) return 0;
				if (d.offset + siz * 2 > d.len) return __LINE__;
				for (size_t i = 0; i < siz; ++i) {
					MapSeries_Pair_t<T> kv;
					if (int r = Read_(d, kv.first, kv.second)) return r;
					v.insert(std::move(kv));
				}
				return 0;
			}
			else {
				return object_interface<T>::Read(*this, d, v);;
			}
			return 0;
		}

		template<typename T, typename ...TS>
		YY_INLINE int Read_(Data_r& d, T& v, TS &...vs) {
			if (auto r = Read_(d, v)) return r;
			return Read_(d, vs...);
		}

	public:
		// 由 object 虚函数 或 不依赖序列化上下文的场景调用
		template<typename...Args>
		YY_INLINE int Read(Data_r& d, Args&...args) {
			static_assert(sizeof...(args) > 0);
			return Read_(d, args...);
		}


		// 向 s 写入数据. 会初始化写入上下文, 并在写入结束后擦屁股( 主要入口 )
		template<typename...Args>
		YY_INLINE void AppendTo(std::string& s, Args const&...args) {
			static_assert(sizeof...(args) > 0);
			(Append_(s, args), ...);
			for (auto&& p : ptrs) {
				*(uint32_t*)p = 0;
			}
			ptrs.clear();
		}

		// 内部函数
		template<typename T>
		YY_INLINE void Append_(std::string& s, T const& v) {
			if constexpr (IsBaseDataType_v<T>) {
				yy::Append(s, v);
			}
			else if constexpr (IsShared_v<T>) {
				using U = typename T::ElementType;
				if (v) {
					if constexpr (std::is_same_v<U, object> || type_id_v<U> > 0) {
						auto h = ((shared_ptr_object_header*)v.pointer - 1);
						if (h->offset == 0) {
							ptrs.push_back(&h->offset);
							h->offset = (uint32_t)ptrs.size();
							Append_(s, *v);
						}
						else {
							s.append(std::to_string(h->offset));
						}
					}
					else {
						Append_(s, *v);
					}
				}
				else {
					s.append("null");
				}
			}
			else if constexpr (IsWeak_v<T>) {
				Append_(s, v.Lock());
			}
			else if constexpr (std::is_base_of_v<object, T>) {
				v.Append(*this, s);
			}
			else if constexpr (IsOptional_v<T>) {
				if (v.has_value()) {
					Append_(s, *v);
				}
				else {
					s.append("null");
				}
			}
			else if constexpr (IsVector_v<T> || IsSetSeries_v<T> || IsQueueSeries_v<T>) {
				s.push_back('[');
				if (!v.empty()) {
					for (auto&& o : v) {
						Append_(s, o);
						s.push_back(',');
					}
					s[s.size() - 1] = ']';
				}
				else {
					s.push_back(']');
				}
			}
			else if constexpr (IsMapSeries_v<T>) {
				s.push_back('[');
				if (!v.empty()) {
					for (auto& kv : v) {
						Append_(s, kv.first);
						s.push_back(',');
						Append_(s, kv.second);
						s.push_back(',');
					}
					s[s.size() - 1] = ']';
				}
				else {
					s.push_back(']');
				}
			}
			else if constexpr (IsPair_v<T>) {
				s.push_back('[');
				Append_(s, v.first);
				s.push_back(',');
				Append_(s, v.second);
				s.push_back(']');
			}
			else if constexpr (IsTuple_v<T>) {
				s.push_back('[');
				std::apply([&](auto const &... args) {
				    (Append(s, args, ','), ...);
					if constexpr (sizeof...(args) > 0) {
						s.resize(s.size() - 1);
					}
					}, v);
				s.push_back(']');
			}
			else {
				object_interface<T>::Append(*this, s, v);
			}
		}

		// 由 object 虚函数 或 不依赖序列化上下文的场景调用
		template<typename...Args>
		YY_INLINE void Append(std::string& s, Args const&...args) {
			static_assert(sizeof...(args) > 0);
			(Append_(s, args), ...);
		}


		// 字符串拼接，方便输出
		template<typename...Args>
		YY_INLINE std::string ToString(Args const&...args) {
			static_assert(sizeof...(args) > 0);
			std::string s;
			AppendTo(s, args...);
			return s;
		}


		// 向 out 深度复制 in. 会初始化 ptrs, 并在写入结束后擦屁股( 主要入口 )
		template<typename T>
		YY_INLINE void CloneTo(T const& in, T& out) {
			Clone_(in, out);
			for (auto& kv : weaks) {
				if (kv.first->offset) {
					auto h = (shared_ptr_object_header*)ptrs2[kv.first->offset - 1] - 1;
					++h->weak_count;
					*kv.second = h;
				}
				else {
					*kv.second = kv.first;
					++kv.first->weak_count;
				}
			}
			for (auto&& p : ptrs) {
				*(uint32_t*)p = 0;
			}
			ptrs.clear();
			ptrs2.clear();
			weaks.clear();
		}

		// 向 out 深度复制 in. 会初始化 ptrs, 并在写入结束后擦屁股( 主要入口 )
		template<typename T>
		YY_INLINE std::decay_t<T> Clone(T const& in) {
			std::decay_t<T> out;
			CloneTo(in, out);
			return out;
		}

		template<class Tuple, std::size_t N>
		struct TupleForeachClone {
			YY_INLINE static void Clone(object_handler& self, Tuple const& in, Tuple& out) {
				self.Clone_(std::get<N - 1>(in), std::get<N - 1>(out));
				TupleForeachClone<Tuple, N - 1>::Clone(self, in, out);
			}
		};

		template<class Tuple>
		struct TupleForeachClone<Tuple, 1> {
			static void Clone(object_handler& self, Tuple const& in, Tuple& out) {}
		};

		template<typename T>
		YY_INLINE void Clone_(T const& in, T& out) {
			if constexpr (IsShared_v<T>) {
				using U = typename T::ElementType;
				if constexpr (std::is_base_of_v<object, U>) {
                    static_assert(std::is_same_v<object, U> || type_id_v<U> > 0);
					if (!in) {
						out.Reset();
					}
					else {
						auto h = ((shared_ptr_object_header*)in.pointer - 1);
						if (h->offset == 0) {
							ptrs.push_back(&h->offset);
							h->offset = (uint32_t)ptrs.size();

							auto inTypeId = in.typeId();
							if (out.typeId() != inTypeId) {
								out = std::move(Create(inTypeId).template ReinterpretCast<U>());
							}
							ptrs2.push_back(out.pointer);
							Clone_(*in, *out);
						}
						else {
							out = *(T*)&ptrs2[h->offset - 1];
						}
					}
				}
				else {
					if (in) {
						out.Emplace();
						Clone_(*in, *out);
					}
					else out.Reset();
				}
			}
			else if constexpr (IsWeak_v<T>) {
				out.Reset();
				if (in.h && in.h->shared_count) {
					weaks.emplace_back(in.h, &out.h);
				}
			}
			else if constexpr (std::is_base_of_v<object, T>) {
				in.Clone(*this, (void*)&out);
			}
			else if constexpr (IsOptional_v<T>) {
				if (in.has_value()) {
					if (!out.has_value()) {
						out.emplace();
					}
					Clone_(*in, *out);
				}
				else {
					out.reset();
				}
			}
			else if constexpr (IsVector_v<T>) {
				auto siz = in.size();
				out.resize(siz);
				if constexpr (IsPod_v<typename T::value_type>) {
					memcpy(out.data(), in.data(), siz * sizeof(typename T::value_type));
				}
				else {
					for (size_t i = 0; i < siz; ++i) {
						Clone_(in[i], out[i]);
					}
				}
			}
			else if constexpr (IsSetSeries_v<T> || IsQueueSeries_v<T>) {
				out.clear();
				for (auto&& o : in) {
                    if constexpr (IsQueueSeries_v<T>) {
                        Clone_(o, out.emplace_back());
                    } else {
                        Clone_(o, out.emplace());
                    }
				}
			}
			else if constexpr (IsTuple_v<T>) {
				TupleForeachClone<T, std::tuple_size_v<T>>::Clone(*this, in, out);
			}
			else if constexpr (IsPair_v<T>) {
				Clone_(out.first, in.first);
				Clone_(out.second, in.second);
			}
			else if constexpr (IsMapSeries_v<T>) {
				out.clear();
				for (auto&& kv : in) {
					std::pair<std::decay_t<decltype(kv.first)>, std::decay_t<decltype(kv.second)>> tar;
					Clone_(kv.first, tar.first);
					Clone_(kv.second, tar.second);
					out.insert(std::move(tar));
				}
			}
			else if constexpr (std::is_same_v<std::string, T> || std::is_base_of_v<Span, T>) {
				out = in;
			}
			else {
				object_interface<T>::Clone(*this, in, out);
			}
		}

		// 斩断循环引用的 shared_ptr 以方便顺利释放内存( 入口 )
		// 并不直接清空 args
		template<typename...Args>
		YY_INLINE void KillRecursive(Args&...args) {
			static_assert(sizeof...(args) > 0);
			(RecursiveReset_(args), ...);
			for (auto&& p : ptrs) {
				*(uint32_t*)p = 0;
			}
			ptrs.clear();
		}

	protected:
		template<typename T>
		YY_INLINE void RecursiveReset_(T& v) {
			if constexpr (IsShared_v<T>) {
				if (v) {
					auto h = ((shared_ptr_object_header*)v.pointer - 1);
					if (h->offset == 0) {
						h->offset = 1;
						ptrs.push_back(&h->offset);
						RecursiveReset_(*v);
					}
					else {
						--h->shared_count;
						v.pointer = nullptr;
					}
				}
			}
			else if constexpr (IsWeak_v<T>) {
			}
			else if constexpr (std::is_base_of_v<object, T>) {
				v.RecursiveReset(*this);
			}
			else if constexpr (IsOptional_v<T>) {
				if (v.has_value()) {
					RecursiveReset_(*v);
				}
			}
			else if constexpr (IsVector_v<T> || IsSetSeries_v<T> || IsQueueSeries_v<T>) {
				for (auto& o : v) {
					RecursiveReset_(o);
				}
			}
			else if constexpr (IsTuple_v<T>) {
				std::apply([this](auto&... args) {
					(RecursiveReset_(args), ...);
					}, v);
			}
			else if constexpr (IsPair_v<T>) {
				RecursiveReset(v.first, v.second);
			}
			else if constexpr (IsMapSeries_v<T>) {
				for (auto& kv : v) {
					RecursiveReset_(kv.second);
				}
			}
			else if constexpr (std::is_same_v<std::string, T> || std::is_base_of_v<Span, T>) {
			}
			else {
				object_interface<T>::RecursiveReset(*this, v);
			}
		}
	public:

		// 供类成员函数调用
		template<typename...Args>
		YY_INLINE void RecursiveReset(Args&...args) {
			static_assert(sizeof...(args) > 0);
			(RecursiveReset_(args), ...);
		}






		// 判断是否存在循环引用，存在则返回非 0 ( 第几个shared_ptr )( 入口 )
		template<typename...Args>
		YY_INLINE int HasRecursive(Args const&...args) {
			static_assert(sizeof...(args) > 0);
			auto r = RecursiveCheck_(args...);
			for (auto&& p : ptrs) {
				*(uint32_t*)p = 0;
			}
			ptrs.clear();
			return r;
		}

	protected:
		template<std::size_t I = 0, typename... Tp>
		YY_INLINE std::enable_if_t<I == sizeof...(Tp) - 1, int> RecursiveCheckTuple(std::tuple<Tp...> const& t) {
			return RecursiveCheck_(std::get<I>(t));
		}

		template<std::size_t I = 0, typename... Tp>
		YY_INLINE std::enable_if_t < I < sizeof...(Tp) - 1, int> RecursiveCheckTuple(std::tuple<Tp...> const& t) {
			if (int r = RecursiveCheck_(std::get<I>(t))) return r;
			return RecursiveCheckTuple<I + 1, Tp...>(t);
		}

		template<typename T>
		YY_INLINE int RecursiveCheck_(T const& v) {
			if constexpr (IsShared_v<T>) {
				if (v) {
					auto h = ((shared_ptr_object_header*)v.pointer - 1);
					if (h->offset == 0) {
						ptrs.push_back(&h->offset);
						h->offset = (uint32_t)ptrs.size();
						return RecursiveCheck_(*v);
					}
					else return h->offset;
				}
			}
			else if constexpr (IsWeak_v<T>) {
			}
			else if constexpr (std::is_base_of_v<object, T>) {
				return v.RecursiveCheck(*this);
			}
			else if constexpr (IsOptional_v<T>) {
				if (v.has_value()) {
					return RecursiveCheck_(*v);
				}
			}
			else if constexpr (IsVector_v<T> || IsSetSeries_v<T> || IsQueueSeries_v<T>) {
				for (auto& o : v) {
					if (int r = RecursiveCheck_(o)) return r;
				}
			}
			else if constexpr (IsTuple_v<T>) {
				return RecursiveCheckTuple(v);
			}
			else if constexpr (IsPair_v<T>) {
				return RecursiveCheck(v.first, v.second);
			}
			else if constexpr (IsMapSeries_v<T>) {
				for (auto& kv : v) {
					if (int r = RecursiveCheck_(kv.second)) return r;
				}
			}
			else if constexpr (std::is_same_v<std::string, T> || std::is_base_of_v<Span, T>) {
			}
			else {
				return object_interface<T>::RecursiveCheck(*this, v);
			}
			return 0;
		}

		template<typename T, typename ...TS>
		YY_INLINE int RecursiveCheck_(T const& v, TS const&...vs) {
			if (auto r = RecursiveCheck_(v)) return r;
			return RecursiveCheck_(vs...);
		}

	public:

		// 供类成员函数调用
		template<typename...Args>
		YY_INLINE int RecursiveCheck(Args const&...args) {
			static_assert(sizeof...(args) > 0);
			return RecursiveCheck_(args...);
		}







		// 设置默认值( 主要针对 类，结构体 )
		template<typename...Args>
		YY_INLINE void SetDefaultValue(Args&...args) {
			static_assert(sizeof...(args) > 0);
			(SetDefaultValue_(args), ...);
		}

	protected:
		template<typename T>
		YY_INLINE void SetDefaultValue_(T& v) {
			if constexpr (IsShared_v<T> || IsWeak_v<T>) {
				v.Reset();
			}
			else if constexpr (std::is_base_of_v<object, T>) {
				v.SetDefaultValue(*this);
			}
			else if constexpr (std::is_same_v<Data, T>) {
				v.Clear();
			}
			else if constexpr (IsOptional_v<T>) {
				v.reset();
			}
			else if constexpr (IsVector_v<T> || IsSetSeries_v<T> || IsQueueSeries_v<T> || IsMapSeries_v<T> || std::is_same_v<T, std::string>) {
				v.clear();
			}
			else if constexpr (IsTuple_v<T>) {
				std::apply([&](auto const &... args) {
					(SetDefaultValue_(args), ...);
					}, v);
			}
			else if constexpr (IsPair_v<T>) {
				SetDefaultValue(v.first, v.second);
			}
			else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
				v = (T)0;
			}
			else {
				object_interface<T>::SetDefaultValue(*this, v);
			}
		}


		/************************************************************************************/
		// 各种 Cout
		/************************************************************************************/
	public:

		// 替代 std::cout. 支持实现了 StringFuncs 模板适配的类型
		template<typename...Args>
		inline void Cout(Args const& ...args) {
			std::string s;
			AppendTo(s, args...);
			for (auto&& c : s) {
				if (!c) c = '^';
			}
			std::cout << s;
		}

		// 在 Cout 基础上添加了换行
		template<typename...Args>
		inline void CoutN(Args const& ...args) {
			Cout(args...);
			std::cout << std::endl;
		}

		// 在 CoutN 基础上于头部添加了时间
		template<typename...Args>
		inline void CoutTN(Args const& ...args) {
			std::cout << "[" << yy::ToString(std::chrono::system_clock::now()) << "] ";
			CoutN(args...);
		}

		// 立刻输出
		inline void CoutFlush() {
			std::cout.flush();
		}
	};
}



#define YY_OBJ_OBJECT_H(T, BT) \
using BaseType = BT; \
T() = default; \
T(T const&) = default; \
T& operator=(T const&) = default; \
T(T&& o) = default; \
T& operator=(T&& o) = default; \
void Write(yy::object_handler& o, yy::Data& d) const override; \
int Read(yy::object_handler& o, yy::Data_r& d) override; \
void Append(yy::object_handler& o, std::string& s) const override; \
void AppendCore(yy::object_handler& o, std::string& s) const override; \
void Clone(yy::object_handler& o, void* const& tar) const override; \
int RecursiveCheck(yy::object_handler& o) const override; \
void RecursiveReset(yy::object_handler& o) override; \
void SetDefaultValue(yy::object_handler& o) override;

#define YY_OBJ_STRUCT_H(T) \
T() = default; \
T(T const&) = default; \
T& operator=(T const&) = default; \
T(T&& o) = default; \
T& operator=(T&& o) = default; \
bool operator==(T const&) const = default;

#define YY_OBJ_STRUCT_TEMPLATE_H(T) \
template<> \
struct object_interface<T, void> { \
static void Write(object_handler & om, yy::Data& d, T const& in); \
static void WriteFast(object_handler & om, yy::Data& d, T const& in); \
static int Read(object_handler & om, yy::Data_r& d, T & out); \
static void Append(object_handler & om, std::string& s, T const& in); \
static void AppendCore(object_handler & om, std::string& s, T const& in); \
static void Clone(object_handler & om, T const& in, T & out); \
static int RecursiveCheck(object_handler & om, T const& in); \
static void RecursiveReset(object_handler & om, T & in); \
static void SetDefaultValue(object_handler & om, T & in); \
};
