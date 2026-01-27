#pragma once

//TODO: add a actual pool

template<typename T>
struct IdPool
{
	T take()
	{
		return ++next;
	}

	void release(T t)
	{

	}

	void reset(T newNext)
	{
		next = newNext;
	}

private:

	T next{};
};