/*!
 * \file NoDestructor.h
 *
 * \author czy
 * \date 2023.07.14
 *
 * 
 */
#pragma once
#include <type_traits>
#include <utility>

namespace CDB{

template<typename InstanceType>
class NoDestructor{
public:
	template<typename... ConstructorArgTypes>
	explicit NoDestructor(ConstructorArgTypes&&... constructor_args){
		static_assert(sizeof(instance_storage_) >= sizeof(InstanceType)
			,"instance_storage_ is not large enough to hold the instance");
		static_assert(alignof(decltype(instance_storage_)) >= alignof(InstanceType)
			, "instance_storage_ doesnt match the instance's requirement");
		new(&instance_storage_) InstanceType(std::forward<ConstructorArgTypes>(constructor_args)...);
	}

	~NoDestructor() = default;

	NoDestructor(const NoDestructor&) = delete;
	
	NoDestructor& operator=(const NoDestructor&) = delete;

	InstanceType* get(){
		return reinterpret_cast<InstanceType*>(&instance_storage_);
	}

private:
	typename std::aligned_storage<sizeof(InstanceType), alignof(InstanceType)>::type instance_storage_;
};

}
