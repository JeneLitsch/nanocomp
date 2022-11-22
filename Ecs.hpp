#pragma once
#include <memory>
#include <vector>
#include <tuple>
#include <bitset>
#include <unordered_map>

namespace ecs {
	template<typename ... Components>
	class Ecs;

	template<typename ... Components>
	class Entity {	
	public:
		Entity(const Entity &) = delete;
		Entity(Entity &&) = delete;

		Entity & operator=(const Entity &) = delete;
		Entity & operator=(Entity &&) = delete;

		Entity(std::uint64_t id, Ecs<Components...> & ecs) : id {id}, ecs{ecs} {}
		~Entity() {
			ecs.by_id.erase(this->id);
		}

		std::uint64_t get_id() const {
			return this->id;
		}



		template<typename Component>
		void add(Component component) {
			this->at<Component>() = std::make_unique<Component>(component);
		}



		template<typename Component>
		void remove(Component) {
			this->at<Component>().reset();
		}



		template<typename Component>
		Component & get() {
			return *this->at<Component>();
		}


		template<typename Component>
		const Component & get() const {
			return *this->at<Component>();
		}



		template<typename Component>
		bool has() const {
			return static_cast<bool>(this->at<Component>());
		}



		void mark_delete() {
			this->to_be_deleted = true;
		}



		bool is_marked_delete() const {
			return this->to_be_deleted;
		}

	private:
		bool to_be_deleted = false;

		template<typename Component>
		std::unique_ptr<Component> & at() {
			return std::get<std::unique_ptr<Component>>(this->components);
		}

		template<typename Component>
		const std::unique_ptr<Component> & at() const {
			return std::get<std::unique_ptr<Component>>(this->components);
		}
	
		std::tuple<std::unique_ptr<Components>...> components;
		std::bitset<sizeof...(Components)> signature;
		std::uint64_t id;
		Ecs<Components...> & ecs;
	};



	template<typename ... Components>
	class Ecs {
		friend Entity<Components...>;
	public:
		using Entity = Entity<Components...>;

		Entity & new_entity() {
			const auto id = next_id++;
			this->entities.push_back(std::make_unique<Entity>(id, *this));
			this->by_id[id] = this->entities.back().get();
			return *this->entities.back();
		}



		Entity & get_by_id(std::uint64_t id) {
			return *this->by_id[id];
		}



		bool contains(std::uint64_t id) {
			return this->by_id.contains(id);
		}



		void clean_up() {
			// Move to expired entities to the end of the array
			auto to_be_deleted = std::remove_if(
				std::begin(entities),
				std::end(entities),
				[] (auto & entity) { return entity->is_marked_delete(); }
			);

			// Actually destroy entities 
			this->entities.erase(to_be_deleted, std::end(entities));
		}



		void run_system(auto && system) {
			for(auto & entity : this->entities) {
				system(*entity);
			}
		}

		~Ecs() {
			this->entities.clear();
		}
	private:
		std::vector<std::unique_ptr<Entity>> entities;
		std::unordered_map<std::uint64_t, Entity *> by_id;
		std::uint64_t next_id = 0;
	};
}