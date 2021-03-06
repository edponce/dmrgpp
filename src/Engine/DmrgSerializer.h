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

/*! \file DmrgSerializer.h
 *
 *  Serialize dmrg data
 */
#ifndef DMRG_SERIAL_H
#define DMRG_SERIAL_H

#include "Utils.h"
#include "BLAS.h"
#include "IoSimple.h"

namespace Dmrg {
	//! Move also checkpointing from DmrgSolver to here (FIXME)
	template<typename RealType,typename VectorType,typename MatrixType,typename BasisType,
 		typename FermionSignType> // <-- this last type should be gotten from BasisType in the future FIXME
	class DmrgSerializer {
		public:
			DmrgSerializer(
				const FermionSignType& fs,
				const BasisType& pS,
				const BasisType& pE,
				const BasisType& pSE,
				const VectorType& wf,
				const MatrixType& transform,
				size_t direction)
			: fs_(fs),
			  pS_(pS),
			  pE_(pE),
			  pSE_(pSE),
			  wavefunction_(wf),
			  transform_(transform),
			  direction_(direction)
			{}
			
			
			DmrgSerializer(typename IoSimple::In& io,bool bogus = false) 
			: fs_(io,bogus),pS_(io,"",bogus),pE_(io,"",bogus),pSE_(io,"",bogus)
			{
				if (bogus) return;
				std::string s = "#WAVEFUNCTION_sites=";
				wavefunction_.load(io,s);
				s = "#TRANSFORM_sites=";
				io.readMatrix(transform_,s);
				s = "#DIRECTION=";
				int x = 0;
				io.readline(x,s);
				if (x<0) throw std::runtime_error("DmrgSerializer:: direction must be non-negative\n");
				direction_ = x;
			}
			
			// Save to disk everything needed to compute any observable (OBSOLETE!!)
			template<typename IoOutputter>
			void save(IoOutputter& io) const
			{
				fs_.save(io);
				pS_.save(io);
				pE_.save(io);
				pSE_.save(io);
	
				// save wavefunction
				std::string label = "#WAVEFUNCTION_sites=";
				for (size_t i=0;i<pSE_.block().size();i++) {
					label += utils::ttos(pSE_.block()[i])+",";
				}
				//SparseVector<typename TargettingType::TargetVectorType::value_type> psiSparse(target.gs());
				wavefunction_.save(io,label);
			
				label = "#TRANSFORM_sites=";
				for (size_t i=0;i<pS_.block().size();i++) {
					label += utils::ttos(pS_.block()[i])+",";
				}
				io.printMatrix(transform_,label);
				std::string s = "#DIRECTION="+utils::ttos(direction_);
				io.printline(s);
			}
			
			const FermionSignType& fermionicSign() const { return fs_; }
			
			const BasisType& basisS() const { return pS_; }
			
			const BasisType& basisE() const { return pE_; }
			
			const BasisType& basisSE() const { return pSE_; }
			
			const VectorType& wavefunction() const { return wavefunction_; }
			
			size_t columns() const { return transform_.n_col(); }
			
			size_t rows() const { return transform_.n_row(); }
			
			size_t direction() const { return direction_; }
			
			void transform(MatrixType& ret,const MatrixType& O) const
			{
				//typedef typename MatrixType::value_type FieldType;
				int nBig = O.n_row();
				int nSmall = transform_.n_col();
				MatrixType fmTmp(nSmall,nBig);
				typename MatrixType::value_type alpha=1.0,beta=0.0;
			
				psimag::BLAS::GEMM('N','N',nBig,nSmall,nBig,alpha,
						&(O(0,0)),nBig,&(transform_(0,0)),nBig,beta,&(fmTmp(0,0)),nBig);
				
				psimag::BLAS::GEMM('C','N',nSmall,nSmall,nBig,alpha,
						&(transform_(0,0)),nBig,&(fmTmp(0,0)),nBig,beta,&(ret(0,0)),nSmall);
				
			}
			
		private:
			FermionSignType fs_;
			BasisType pS_;
			BasisType pE_;
			BasisType pSE_;
			VectorType wavefunction_;
			MatrixType transform_;
			size_t direction_;
	}; // class DmrgSerializer
} // namespace Dmrg 

/*@}*/
#endif
