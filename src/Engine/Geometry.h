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
/** \ingroup DMRG */
/*@{*/

/*! \file Geometry.h
 *
 *  DOC NEEDED FIXME
 */
#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "Utils.h"
#include "GeometryTerm.h"

namespace Dmrg {
	
	template<typename RealType>
	class Geometry {
		public:
			typedef GeometryTerm<RealType> GeometryTermType;
			typedef std::vector<size_t> BlockType;

			template<typename IoInputter>
			Geometry(IoInputter& io)
			{
				int x;
				io.readline(x,"TotalNumberOfSites=");
				if (x<0) throw std::runtime_error("TotalNumberOfSites<0 is an error\n");
				std::cerr<<"TotalNumberOfSites "<<x<<"\n";
				linSize_ = x;

				io.readline(x,"NumberOfTerms=");
				std::cerr<<"NumberOfTerms "<<x<<"\n";
				if (x<0) throw std::runtime_error("NumberOfTerms<0 is an error\n");

				for (size_t i=0;i<size_t(x);i++) {
					GeometryTermType t(io,i,linSize_);
					terms_.push_back(t);
				}
			}
			
			size_t connectionKind(size_t smax,size_t ind,size_t jnd) const
			{
				size_t middle = smax + 1;
				if (ind<middle && jnd>=middle) return ProgramGlobals::SYSTEM_ENVIRON;
				if (jnd<middle && ind>=middle) return ProgramGlobals::ENVIRON_SYSTEM;
				if (ind<middle) return ProgramGlobals::SYSTEM_SYSTEM;
				return ProgramGlobals::ENVIRON_ENVIRON;
			}
			
			RealType operator()
				(size_t smax,size_t emin,
				 size_t i1,size_t edof1,size_t i2, size_t edof2,size_t term) const
			{
				//std::cerr<<"smax="<<smax<<" emin="<<emin<<"\n";
				if (smax+1==emin) return terms_[term](i1,edof1,i2,edof2);
				return terms_[term](smax,emin,i1,edof1,i2,edof2);
			}
			
			RealType operator()
				(size_t i1,size_t edof1,size_t i2, size_t edof2,size_t term) const
			{
				return terms_[term](i1,edof1,i2,edof2);
			}
			
// 			const RealType& defaultConnector
// 				(size_t edof1,size_t edof2,size_t term) const
// 			{
// 				return terms_[term].defaultConnector(edof1,edof2);
// 			}
			
			size_t terms() const { return terms_.size(); }
			
			size_t numberOfSites() const { return linSize_; }
			
			void split(BlockType& S,std::vector<BlockType>& X,std::vector<BlockType>& Y,BlockType& E) const
			{
				size_t middle = linSize_/2;
				S.push_back(0);
				for (size_t i=1;i<middle;i++) {
					std::vector<size_t> tmpV(1);
					tmpV[0] = i;
					X.push_back(tmpV);
				}
				
				for (int j=linSize_-2;j>=int(middle);j--) {
					std::vector<size_t> tmpV(1);
					tmpV[0] = j;
					Y.push_back(tmpV);
				}
				
				E.push_back(linSize_-1);
			}
			
			// should be static
// 			bool connected(size_t i1,size_t i2) const
// 			{
// 				return terms_[0].connected(i1,i2); // any term will do
// 			}
// 			
			// should be static
			bool connected(size_t smax,size_t emin,size_t i1,size_t i2) const
			{
				if (smax+1==emin) return terms_[0].connected(i1,i2); // any term will do
				return terms_[0].connected(smax,emin,i1,i2); // any term will do
			}
		private:
			
		
			size_t linSize_;
			std::vector<GeometryTermType> terms_;
			
	}; // class Geometry
} // namespace Dmrg 

/*@}*/
#endif // GEOMETRY_H
