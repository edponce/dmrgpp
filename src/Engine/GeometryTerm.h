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

/*! \file GeometryTerm.h
 *
 * FIXME DOC
 */
#ifndef GEOMETRY_TERM_H
#define GEOMETRY_TERM_H

#include "Utils.h"
#include "GeometryDirection.h"

namespace Dmrg {
	
	template<typename RealType>
	class GeometryTerm {
		public:
			typedef GeometryDirection<RealType> GeometryDirectionType;
			
			template<typename IoInputter>
			GeometryTerm(IoInputter& io,size_t termId,size_t linSize)
			: linSize_(linSize),leg_(1)
			{
				int x;
				io.readline(x,"DegreesOfFreedom=");
				if (x<=0) throw std::runtime_error("DegreesOfFreedom<=0 is an error\n");
				std::cerr<<"DegreesOfFreedom "<<x<<"\n";
				edof_ = x;
				std::string s;
				io.readline(s,"GeometryKind=");
				std::cerr<<"GeometryKind "<<s<<"\n";
				geometryKind_=getGeometry(s);
				std::string gOptions;
				io.readline(gOptions,"GeometryOptions=");
				std::cerr<<"GeometryOptions "<<gOptions<<"\n";
				size_t dirs = 0;
				switch (geometryKind_) {
					case GeometryDirectionType::LADDER:
						dirs = 2;
						// no break here!
					case GeometryDirectionType::LADDERX:
						if (dirs==0) dirs=4;
						io.readline(x,"LadderLeg=");
						if (x<2) throw std::runtime_error("LadderLeg<2 is an error\n");
						leg_=x;
						break;
					case GeometryDirectionType::CHAIN:
						dirs = 1;
				}
				for (size_t i=0;i<dirs;i++) {
					GeometryDirectionType gd(io,i,linSize_,edof_,geometryKind_,leg_,gOptions);
					directions_.push_back(gd);
				}
				
				cachedValues_.resize(linSize_*linSize_*edof_*edof_);
				
				for (size_t i1=0;i1<linSize_;i1++)
					for (size_t i2=0;i2<linSize_;i2++) 
						for (size_t edof1=0;edof1<edof_;edof1++)
							for (size_t edof2=0;edof2<edof_;edof2++)
								cachedValues_[pack(i1,edof1,i2,edof2)]=
									calcValue(i1,edof1,i2,edof2);
				//std::cerr<<cachedValues_;
			}
			
			const RealType& operator()(size_t i1,size_t edof1,size_t i2,size_t edof2) const
			{
				size_t p = pack(i1,edof1,i2,edof2);
				return cachedValues_[p];
			}
			
			//assumes 1<smax+1 < emin
			const RealType& operator()(size_t smax,size_t emin,
				size_t i1,size_t edof1,size_t i2,size_t edof2) const
			{
				bool bothFringe = (fringe(i1,smax,emin) && fringe(i2,smax,emin));
				size_t siteNew1 = i1;
				size_t siteNew2 = i2;
				size_t edofNew1 = edof1;
				size_t edofNew2 = edof2;
				if (bothFringe) {
					if (i2<i1) {
						siteNew1 = i2;
						siteNew2 = i1;
						edofNew1 = edof2;
						edofNew2 = edof1;
					}
					siteNew2 = getSubstituteSite(smax,emin,siteNew2);
				}
				
				size_t p = pack(siteNew1,edofNew1,siteNew2,edofNew2);
				return cachedValues_[p];
			}
			
// 			const RealType& defaultConnector
// 				(size_t i1,size_t edof1,size_t i2,size_t edof2) const
// 			{
// 				size_t dir = calcDir(i1,i2);
// 				return directions_[dir].defaultConnector(edof1,edof2);
// 			}
			
			bool connected(size_t smax,size_t emin,size_t i1,size_t i2) const
			{
				if (i1==i2) return false;
				bool bothFringe = (fringe(i1,smax,emin) && fringe(i2,smax,emin));
				if (!bothFringe) return connected(i1,i2);
				//std::cerr<<"fringe= "<<i1<<" "<<i2<<"\n";
				return true;
			}
			
			// should be static and private
			bool connected(size_t i1,size_t i2) const
			{
				if (i1==i2) return false;
				switch (geometryKind_) {
					case GeometryDirectionType::CHAIN:
						return neighbors(i1,i2);
						break;
					case GeometryDirectionType::LADDERX:
					case GeometryDirectionType::LADDER:
						size_t c1 = i1/leg_;
						size_t c2 = i2/leg_;
						size_t r1 = i1%leg_;
						size_t r2 = i2%leg_;
						if (c1==c2) return neighbors(r1,r2);
						if (r1==r2) return neighbors(c1,c2);
						return (geometryKind_==GeometryDirectionType::LADDERX
							&& neighbors(r1,r2) && neighbors(c1,c2));
				}
				throw std::runtime_error("Unknown geometry\n");
			}
			
		private:	
			RealType calcValue(size_t i1,size_t edof1,size_t i2,size_t edof2) const
			{
				if (!connected(i1,i2)) return 0.0;

				size_t dir = calcDir(i1,i2);
				if (directions_[dir].constantValues()) {
					return directions_[dir](edof1,edof2);
				}
				
				return directions_[dir](i1,edof1,i2,edof2);
			}
			
			size_t pack(size_t i1,size_t edof1,size_t i2,size_t edof2) const
			{
				return edof1+i1*edof_+(edof2+i2*edof_)*linSize_*edof_;
			}
			
			size_t getGeometry(const std::string& s) const
			{
				size_t x = 0;
				if (s=="chain") x=GeometryDirectionType::CHAIN;
				else if (s=="ladder") x=GeometryDirectionType::LADDER;
				else if (s=="ladderx") x=GeometryDirectionType::LADDERX;
				else throw std::runtime_error("unknown geometry\n");
				return x;
			}
			
			size_t calcDir(size_t i1,size_t i2) const
			{
				switch (geometryKind_) {
					case GeometryDirectionType::CHAIN:
						return GeometryDirectionType::DIRECTION_X;	
					case GeometryDirectionType::LADDER:
						if (sameColumn(i1,i2)) return GeometryDirectionType::DIRECTION_Y;
						return GeometryDirectionType::DIRECTION_X;
					case GeometryDirectionType::LADDERX:
						if (sameColumn(i1,i2)) return GeometryDirectionType::DIRECTION_Y;
						if (sameRow(i1,i2)) return GeometryDirectionType::DIRECTION_X;
						size_t imin = (i1<i2) ? i1 : i2;
						if (imin&1) return GeometryDirectionType::DIRECTION_XPY;
						return GeometryDirectionType::DIRECTION_XMY;
				}
				throw std::runtime_error("Unknown geometry\n");
			}
			
			bool sameColumn(size_t i1,size_t i2) const
			{
				size_t c1 = i1/leg_;
				size_t c2 = i2/leg_;
				if (c1 == c2) return true;
				return false;
			}
			
			bool sameRow(size_t i1,size_t i2) const
			{
				size_t c1 = i1%leg_;
				size_t c2 = i2%leg_;
				if (c1 == c2) return true;
				return false;
			}
			
			bool neighbors(size_t i1,size_t i2) const
			{
				return (i1-i2==1 || i2-i1==1);
			}
			
			bool fringe(size_t i,size_t smax,size_t emin) const
			{
				bool a,b;
				switch (geometryKind_) {
					case GeometryDirectionType::CHAIN:
						return (i==smax || i==emin);
					case GeometryDirectionType::LADDER:
						a = (i<emin && i>=smax-1);
						b = (i>smax && i<=emin+1);
						return (a || b);
					case GeometryDirectionType::LADDERX:
						a = (i<emin && i>=smax-1);
						b = (i>smax && i<=emin+1);
						if (smax & 1) return (a || b);
						a = (i<emin && i>=smax-2);
						b = (i>smax && i<=emin+2);
						return (a || b);
				}
				throw std::runtime_error("Unknown geometry\n");
			}
			
			
			// siteNew2 is fringe in the environment
			size_t getSubstituteSite(size_t smax,size_t emin,size_t siteNew2) const
			{
				switch (geometryKind_) {
					case GeometryDirectionType::CHAIN:
						return smax+1;
					case GeometryDirectionType::LADDER:
						return smax+siteNew2-emin+1;
					case GeometryDirectionType::LADDERX:
						return smax+siteNew2-emin+1;
				}
				throw std::runtime_error("Unknown geometry\n");
			}
			
			size_t linSize_;
			size_t leg_;
			size_t edof_;
			size_t geometryKind_;
			std::vector<GeometryDirectionType> directions_;
			std::vector<RealType> cachedValues_;
	}; // class GeometryTerm
} // namespace Dmrg

/*@}*/
#endif // GEOMETRY_TERM_H
