#ifndef _XECALLQUEUE_HPP
#define _XECALLQUEUE_HPP
/*-------------------------------------------------------------------------
 * drawElements Quality Program Test Executor
 * ------------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Cross-thread function call dispatcher.
 *//*--------------------------------------------------------------------*/

#include "xeDefs.hpp"
#include "deMutex.hpp"
#include "deSemaphore.hpp"
#include "deRingBuffer.hpp"

#include <vector>

namespace xe
{

class Call;
class CallReader;
class CallWriter;
class CallQueue;

// \todo [2012-07-10 pyry] Optimize memory management in Call
// \todo [2012-07-10 pyry] CallQueue API could be improved to match TestLog API more closely.
//						   In order to do that, reference counting system for call object management is needed.

class Call
{
public:
	typedef void (*Function) (CallReader data);

								Call				(void);
								~Call				(void);

	void						clear				(void);

	Function					getFunction			(void) const	{ return m_func;				}
	void						setFunction			(Function func)	{ m_func = func;				}

	int							getDataSize			(void) const	{ return (int)m_data.size();	}
	void						setDataSize			(int size)		{ m_data.resize(size);			}

	const deUint8*				getData				(void) const	{ return m_data.empty() ? DE_NULL : &m_data[0];	}
	deUint8*					getData				(void)			{ return m_data.empty() ? DE_NULL : &m_data[0];	}

private:
	Function					m_func;
	std::vector<deUint8>		m_data;
};

class CallReader
{
public:
					CallReader			(Call* call);
					CallReader			(void) : m_call(DE_NULL), m_curPos(0) {}

	void			read				(deUint8* bytes, int numBytes);
	const deUint8*	getDataBlock		(int numBytes);					//!< \note Valid only during call.

private:
	Call*			m_call;
	int				m_curPos;
};

class CallWriter
{
public:
					CallWriter			(CallQueue* queue, Call::Function function);
					~CallWriter			(void);

	void			write				(const deUint8* bytes, int numBytes);
	void			enqueue				(void);

private:
					CallWriter			(const CallWriter& other);
	CallWriter&		operator=			(const CallWriter& other);

	CallQueue*		m_queue;
	Call*			m_call;
	bool			m_enqueued;
};

class CallQueue
{
public:
							CallQueue			(void);
							~CallQueue			(void);

	void					callNext			(void); //!< Executes and removes first call in queue. Will block if queue is empty.

	Call*					getEmptyCall		(void);
	void					enqueue				(Call* call);
	void					freeCall			(Call* call);

private:
							CallQueue			(const CallQueue& other);
	CallQueue&				operator=			(const CallQueue& other);

	de::Semaphore			m_callSem;

	de::Mutex				m_lock;
	std::vector<Call*>		m_calls;
	std::vector<Call*>		m_freeCalls;
	de::RingBuffer<Call*>	m_callQueue;
};

// Stream operators for call reader / writer.

CallReader&		operator>>	(CallReader& reader, std::string& value);
CallWriter&		operator<<	(CallWriter& writer, const char* str);

template <typename T>
CallReader& operator>> (CallReader& reader, T& value)
{
	reader.read((deUint8*)&value, sizeof(T));
	return reader;
}

template <typename T>
CallWriter& operator<< (CallWriter& writer, T& value)
{
	writer.write((const deUint8*)&value, sizeof(T));
	return writer;
}

} // xe

#endif // _XECALLQUEUE_HPP
