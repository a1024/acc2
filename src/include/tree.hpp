#pragma once
#ifndef STD_TREE//non-standard
#define STD_TREE
#include	"vector"
#include	"map"
#include	"assert.h"
namespace	std
{
	template<typename T>struct node
	{
		T data;
		vector<node*> children;

		node():data(){}
		node(node const &other):data(other.data), children(other.children){}
		node(node &&other):data((T&&)other.data), children((Container&&)other.children){}
		node(T const &data):data(data){}
		~node()
		{
			for(int k=0;k<children.size();++k)
			{
				delete children[k];
				children[k]=nullptr;
			}
		}
		node& operator=(node const &other)
		{
			if(this!=&other)
			{
				data=other.data;
				children=other.children;
			}
			return *this;
		}
		node& operator=(node &&other)
		{
			if(this!=&other)
			{
				data=(T&&)other.data;
				children=(Container&&)other.children;
			}
			return *this;
		}

		void push_back(T const &child)
		{
			children.push_back(new node(child));
		}
		void insert(T const &child, size_t position)
		{
			children.insert(position, new node(child));
		}
		void pop_back()
		{
			assert(children.size()>0);
			
			delete children.back();
			children.pop_back();
		}
	};
	template<typename T, typename K, typename LessThan=less<K>>struct mapnode
	{
		typedef map<K, mapnode<T, K, LessThan>*, LessThan> Container;
		//typedef map<K, mapnode*, LessThan> Container;

		T data;
		//mapnode *parent;//space-ineffieient
		Container children;

		mapnode():data(){}
		mapnode(mapnode const &other):data(other.data), children(other.children){}
		mapnode(mapnode &&other):data((T&&)other.data), children((Container&&)other.children){}
		mapnode(T const &data):data(data){}
		~mapnode()
		{
			Container::EType *p=children.data();
			for(size_t k=0;k<children.size();++k)
			{
				delete p[k].second;
				p[k].second=nullptr;
			}
		}
		mapnode& operator=(mapnode const &other)
		{
			if(this!=&other)
			{
				data=other.data;
				children=other.children;
			}
			return *this;
		}
		mapnode& operator=(mapnode &&other)
		{
			if(this!=&other)
			{
				data=(T&&)other.data;
				children=(Container&&)other.children;
			}
			return *this;
		}

		typename Container::EType* find(K const &key)
		{
			return children.find(key);
		}

		typename Container::EType* insert(K const &key, T const &data)
		{
			return children.insert(Container::EType(key, new mapnode(data)));
		}
		typename Container::EType& insert_no_overwrite(K const &key, bool *old)
		{
			return children.insert_no_overwrite(key, old);
		}
		int erase(K const &key)
		{
			return children.erase(key);
		}
	};
	template<typename T, typename K, typename LessThan=less<K>>struct maptree
	{
		typedef mapnode<T, K, LessThan> Node;
		typedef pair<K*, Node*> PathElement;
		typedef vector<PathElement> Path;

		Node root;
		Path path;
		
		maptree(){}
		maptree(maptree const &other):root(other.root), path(other.path){}
		maptree(maptree &&other):root((Node&&)other.root), path((Path&&)other.path){}
		maptree& operator=(maptree const &other)
		{
			if(this!=&other)
			{
				root=other.root;
				path=other.path;
			}
			return *this;
		}
		maptree& operator=(maptree &&other)
		{
			if(this!=&other)
			{
				root=(Node&&)other.root;
				path=(Path&&)other.path;
			}
			return *this;
		}

		void insert(K const &key, T const &data, bool overwrite)//TODO: report if overwritten
		{
			Node::Container::EType *e=nullptr;
			if(path.size())
				e=path.back().second->insert(key, data);
			else
				e=root.insert(key, data);
			if(e)
				path.push_back(PathElement(&e->first, e->second));
		}
		Node* find_root(K const &key)
		{
			auto e=root.find(key);
			if(e)
				return e->second;
			return nullptr;
		}
		Node* find(K const &key)
		{
			Node::Container::EType *e=nullptr;
			if(path.size())
				e=path.back().second->find(key);
			else
				e=root.find(key);
			if(e)
				return e->second;
			return nullptr;
		}
		Node* open(K const &key)
		{
			Node::Container::EType *e=nullptr;
			if(path.size())
				e=path.back().second->find(key);
			else
				e=root.find(key);
			if(e)
			{
				path.push_back(PathElement(&e->first, e->second));
				return e->second;
			}
			return nullptr;
		}
		void close()
		{
			assert(path.size()>0);
			path.pop_back();
		}
		void close_and_erase()
		{
			assert(path.size()>0);
			auto key=path.back().first;
			if(path.size()>1)
				path[path.size()-1].second->erase(*key);
			else
				root.erase(*key);
			path.pop_back();
		}
		K* get_topmost_key()
		{
			if(path.size()>0)
				return path.back().first;
			return nullptr;
		}
	};
}
#endif