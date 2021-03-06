// BEGIN LICENSE BLOCK
/*
Copyright � 2009 , UT-Battelle, LLC
All rights reserved

[DMRG++, Version 2.0.0]
[by G.A., Oak Ridge National Laboratory]

UT Battelle Open Source Software License 11242008

OPEN SOURCE LICENSE

Subject to the conditions of this License, each
contributor to this software hereby grants, free of
charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), a
perpetual, worldwide, non-exclusive, no-charge,
royalty-free, irrevocable copyright license to use, copy,
modify, merge, publish, distribute, and/or sublicense
copies of the Software.

1. Redistributions of Software must retain the above
copyright and license notices, this list of conditions,
and the following disclaimer.  Changes or modifications
to, or derivative works of, the Software should be noted
with comments and the contributor and organization's
name.

2. Neither the names of UT-Battelle, LLC or the
Department of Energy nor the names of the Software
contributors may be used to endorse or promote products
derived from this software without specific prior written
permission of UT-Battelle.

3. The software and the end-user documentation included
with the redistribution, with or without modification,
must include the following acknowledgment:

"This product includes software produced by UT-Battelle,
LLC under Contract No. DE-AC05-00OR22725  with the
Department of Energy."
 
*********************************************************
DISCLAIMER

THE SOFTWARE IS SUPPLIED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER, CONTRIBUTORS, UNITED STATES GOVERNMENT,
OR THE UNITED STATES DEPARTMENT OF ENERGY BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.

NEITHER THE UNITED STATES GOVERNMENT, NOR THE UNITED
STATES DEPARTMENT OF ENERGY, NOR THE COPYRIGHT OWNER, NOR
ANY OF THEIR EMPLOYEES, REPRESENTS THAT THE USE OF ANY
INFORMATION, DATA, APPARATUS, PRODUCT, OR PROCESS
DISCLOSED WOULD NOT INFRINGE PRIVATELY OWNED RIGHTS.

*********************************************************


*/
// END LICENSE BLOCK
#ifndef DISKSTACK_HEADER_H
#define DISKSTACK_HEADER_H

#include <stack>
#include "IoSimple.h"

//! A disk stack, similar to std::stack but stores in disk not in memory
namespace Dmrg {
	template<typename DataType>
	class DiskStack {
	
			typedef typename IoSimple::In IoInType;
			typedef typename IoSimple::Out IoOutType;
		public:
			DiskStack(std::string const &name,size_t rank=0,bool debug=true) :
				rank_(rank),
				label_(name),
				total_(0),
				debug_(debug)
			{
				debugPrint("Constructor name="+name);
			}
			
			~DiskStack()
			{
				ioOut_.open(label_,std::ios_base::app,rank_);
				ioOut_.printline("#STACKMETARANK="+utils::ttos(rank_));
				
				ioOut_.printline("#STACKMETATOTAL="+utils::ttos(total_));
				ioOut_.printline("#STACKMETADEBUG="+utils::ttos(debug_));
				ioOut_.printStack(stack_,"#STACKMETASTACK");
				ioOut_.close();
			}

			void empty()
			{
				ioOut_.open(label_,std::ios_base::trunc,rank_);
				ioOut_.close();
				total_=0;
				stack_.empty();
			}

			void push(DataType const &d) 
			{
				//std::string tmpLabel = label_ + utils::ttos(total_);
				ioOut_.open(label_,std::ios_base::app,rank_);
				d.save(ioOut_);
				ioOut_.close();

				stack_.push(total_);
				total_++;

				std::string s = "push label_="+label_+" total="+utils::ttos(total_);
				debugPrint(s);
			}

			void pop()
			{
				stack_.pop();
				debugPrint("pop\n");
			}

			DataType top()
			{
				ioIn_.open(label_);
				DataType dt(ioIn_,"",stack_.top());
				debugPrint("top label_="+label_+" stack_.top="+utils::ttos(stack_.top()));
				ioIn_.close();
				return dt;
			}

			size_t size() const { return stack_.size(); }

			void load(const std::string& file)
			{
				try {
					ioIn_.open(file);
				} catch (std::exception& e) {
					std::cerr<<"Problem opening reading file "<<file<<"\n";
					throw std::runtime_error("DiskStack::load(...)\n");
				}
				ioIn_.readline(rank_,"#STACKMETARANK=",IoInType::LAST_INSTANCE);
				label_=file;
				ioIn_.readline(total_,"#STACKMETATOTAL=");
				ioIn_.readline(debug_,"#STACKMETADEBUG=");
				ioIn_.read(stack_,"#STACKMETASTACK");
				ioIn_.close();
			}

		private:
			void debugPrint(const std::string& s)
			{
				if (debug_) std::cerr<<s<<"\n";
			}

			size_t rank_;
			std::string label_;
			int total_;
			bool debug_;
			IoInType ioIn_;
			IoOutType ioOut_;
			std::stack<int> stack_;
	}; // class DiskStack
} // namespace DMrg

#endif
