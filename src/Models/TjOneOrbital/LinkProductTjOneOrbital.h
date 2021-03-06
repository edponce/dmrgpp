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

/*! \file LinkProductTjOneOrbital.h
 *
 *  LinkProduct for TjOneOrbital model
 *
 */
#ifndef LINK_PROD_TJONEORB_H
#define LINK_PROD_TJONEORB_H

#include "LinkProductStruct.h"

namespace Dmrg {
	template<typename FieldType_,typename ModelHelperType>
	class LinkProductTjOneOrbital {
			typedef typename ModelHelperType::SparseMatrixType SparseMatrixType;
			
		public:
			typedef FieldType_ FieldType;
			typedef LinkProductStruct<FieldType> LinkProductStructType;
			
			enum {JVALUES=0,HOPPINGS=1}; // connector value types
			
			LinkProductTjOneOrbital(size_t dof,
				   const ModelHelperType& modelHelper,
				   const LinkProductStructType& lps,
				    std::vector<FieldType>& x,
				   const std::vector<FieldType>& y) : dof_(dof),modelHelper_(modelHelper),lps_(lps),
					x_(x),y_(y)
			{
			}

			void thread_function_(size_t threadNum,size_t blockSize,pthread_mutex_t* myMutex)
			{
				std::vector<FieldType> xtemp(x_.size());
				for (size_t i=0;i<xtemp.size();i++) xtemp[i]=0;
				for (size_t p=0;p<blockSize;p++) {
					size_t ix = threadNum * blockSize + p;
					size_t i=lps_.isaved[ix];
					size_t j=lps_.jsaved[ix];
					size_t dof1=lps_.dof1saved[ix];
					size_t dof2=lps_.dof2saved[ix];
					int type=lps_.typesaved[ix];
					FieldType tmp=lps_.tmpsaved[ix];
					size_t connectionKind = lps_.connectionsaved[ix];
					//if (connectionKind!=0) continue;
					linkProduct(xtemp,y_,i,dof1,j,dof2,type,tmp,modelHelper_,connectionKind);
					
				}
				if (myMutex) pthread_mutex_lock( myMutex);
				for (size_t i=0;i<x_.size();i++) x_[i]+=xtemp[i];
				if (myMutex) pthread_mutex_unlock( myMutex );
				
			}
			
			static size_t calcBond(int i,int sigma,int j,int sigma2,int type,
				FieldType  &val,
				SparseMatrixType &matrixBlock,
				const ModelHelperType& modelHelper,size_t what)
			{
				if (what==JVALUES) calcBondExchange(i,sigma,j,sigma2,type,val,matrixBlock,modelHelper);
				else calcBondHopping(i,sigma,j,sigma2,type,val,matrixBlock,modelHelper);
				return matrixBlock.nonZero();
			}

		private:
			size_t dof_;
			const ModelHelperType& modelHelper_;
			const LinkProductStructType& lps_;
			std::vector<FieldType>& x_;
			const std::vector<FieldType>& y_;
				
			//! Adds an Exchange bond between system and environment
			static size_t calcBondExchange(int i,int sigma,int j,int sigma2,int type,
					FieldType  val,
					SparseMatrixType &matrix,
					const ModelHelperType& modelHelper) 
			{
				if (sigma!=sigma2 || sigma!=0) return 0;
				int const SystemEnviron=1,EnvironSystem=2;
				int offset = modelHelper.basis2().block().size();
				SparseMatrixType matrixBlock;
				size_t of=1;
				if (modelHelper.isSu2()) {
					of=0;
					val = -val;
				}
				FieldType valOver2 = val*0.5;
				size_t angularMomentum=2;

				if (type==SystemEnviron) {
					const SparseMatrixType& A=modelHelper.getReducedOperator('N',i,0,ModelHelperType::System);
					const SparseMatrixType& B=modelHelper.getReducedOperator('N',j-offset,0,ModelHelperType::Environ);
					SparseMatrixType Aconj=modelHelper.getReducedOperator('C',i,0,ModelHelperType::System);
					SparseMatrixType Bconj=modelHelper.getReducedOperator('C',j-offset,0,ModelHelperType::Environ);
				
					// 0.5*S^+_i S^-_j
					FieldType angularFactor = -1.0;	
					modelHelper.fastOpProdInter(A,Bconj,type,valOver2,matrix,false,angularMomentum,angularFactor,2);
					
					// 0.5*S^-_i S^+_j
					angularFactor = -1.0;	
					modelHelper.fastOpProdInter(Aconj,B,type,valOver2,matrixBlock,false,angularMomentum,angularFactor,0);
					matrix += matrixBlock;
					
					// S^z_i*S^z_j
					const SparseMatrixType& Az=modelHelper.getReducedOperator('N',i,of,ModelHelperType::System);
					const SparseMatrixType& Bz=modelHelper.getReducedOperator('C',j-offset,of,ModelHelperType::Environ);
					angularFactor = 1.0/2.0; // needs to be in sync with LinkProductTjOneOrbital.h
					modelHelper.fastOpProdInter(Az,Bz,type,val,matrixBlock,false,angularMomentum,angularFactor,1);
					matrix += matrixBlock;
					
				} else {	
					if (type!=EnvironSystem) std::cerr<<"EEEEEEEERRRRRRRRRRRROOOOOOOOOOORRRRRRRRRRR\n";
				
					const SparseMatrixType& A=modelHelper.getReducedOperator('N',i-offset,0,ModelHelperType::Environ);
					const SparseMatrixType& B=modelHelper.getReducedOperator('N',j,0,ModelHelperType::System);
					SparseMatrixType Aconj=modelHelper.getReducedOperator('C',i-offset,0,ModelHelperType::Environ);
					SparseMatrixType Bconj=modelHelper.getReducedOperator('C',j,0,ModelHelperType::System);
					
					// 0.5*S^+_i S^-_j	
					FieldType angularFactor = -1.0;
					modelHelper.fastOpProdInter(A,Bconj,type,valOver2,matrix,false,angularMomentum,angularFactor,2);
					
					// 0.5*S^-_i S^+_j
					angularFactor = -1.0;
					modelHelper.fastOpProdInter(Aconj,B,type,valOver2,matrixBlock,false,angularMomentum,angularFactor,0);
					matrix += matrixBlock;
					
					// S^z_i*S^z_j
					const SparseMatrixType& Az=modelHelper.getReducedOperator('N',i-offset,of,ModelHelperType::Environ);
					const SparseMatrixType& Bz=modelHelper.getReducedOperator('C',j,of,ModelHelperType::System);
					angularFactor = 1.0/2.0; // needs to be in sync with LinkProductTjOneORbital.h
					modelHelper.fastOpProdInter(Az,Bz,type,val,matrixBlock,false,angularMomentum,angularFactor,1);
					matrix += matrixBlock;
				}
				return matrix.nonZero();
			}
			
			//! Adds a tight-binding bond between system and environment
			static size_t calcBondHopping(int i,int sigma,int j,int sigma2,int type,
				FieldType  &val,
				SparseMatrixType &matrix,
				const ModelHelperType& modelHelper) 
			{
				
				if (sigma!=sigma2) return 0;
				size_t of = 2;
				int const SystemEnviron=1,EnvironSystem=2;
				//int k=sigma + i*pSprime.dof();
				//int k2=sigma2 + j*pSprime.dof();
				int offset = modelHelper.basis2().block().size();
				size_t angularMomentum=1;
				FieldType angularFactor  = 1.0;
				if (sigma==1) angularFactor  = -1.0;
				SparseMatrixType matrixBlock;
				
				if (type==SystemEnviron) {
					
// 					A=modelHelper.basis2().getOperator(i,sigma).data;
// 					transposeConjugate(B,modelHelper.basis3().getOperator(j-offset,sigma).data);
// 					angularMomentum=0;
//					modelHelper.fastOpProdInter(A,B,type,val,matrixBlock,true);
					const SparseMatrixType& A=modelHelper.getReducedOperator('N',i,of+sigma,ModelHelperType::System);
					const SparseMatrixType& B=
							modelHelper.getReducedOperator('C',j-offset,of+sigma2,ModelHelperType::Environ);
					modelHelper.fastOpProdInter(A,B,type,val,matrixBlock,true,angularMomentum,angularFactor,sigma);
							
					
				} else {
					if (type!=EnvironSystem) std::cerr<<"ERRRRRRRRRRORRRRRRRR\n";
// 					A=modelHelper.basis3().getOperator(i-offset,sigma).data;
// 					transposeConjugate(B,modelHelper.basis2().getOperator(j,sigma).data);
// 					modelHelper.fastOpProdInter(A,B,type,val,matrixBlock,true);
					const SparseMatrixType& A=modelHelper.getReducedOperator('N',i-offset,of+sigma,
							ModelHelperType::Environ);
					const SparseMatrixType& B=modelHelper.getReducedOperator('C',j,sigma2+of,ModelHelperType::System);
					modelHelper.fastOpProdInter(A,B,type,val,matrixBlock,true,angularMomentum,angularFactor,sigma);
					
				}
				
				matrix += matrixBlock;
				return matrix.nonZero();
				
			}
		
		void linkProduct(std::vector<FieldType> &x,std::vector<FieldType> const &y,
					int i,int dof1,int j,int dof2,int type,
			     FieldType  val,
			     const ModelHelperType& modelHelper,size_t what)  const
		{
			if (what==JVALUES) linkProductExchange(x,y,i,dof1,j,dof2,type,val,modelHelper);
			else linkProductHopping(x,y,i,dof1,j,dof2,type,val,modelHelper);
		}
		
		void linkProductExchange(std::vector<FieldType> &x,std::vector<FieldType> const &y,
					int i,int dof1,int j,int dof2,int type,
			     FieldType  val,
			     const ModelHelperType& modelHelper)  const
		{
			if (dof1!=dof2 || dof1!=0) return;
			int const SystemEnviron=1,EnvironSystem=2;
			int offset = modelHelper.basis2().block().size();
			size_t of=1;
			if (modelHelper.isSu2()) {
				of=0;
				val = -val;
			}
			FieldType valOver2 = val*0.5;
			size_t angularMomentum=2;

			if (type==SystemEnviron) {
				const SparseMatrixType& A=modelHelper.getReducedOperator('N',i,0,ModelHelperType::System);
				const SparseMatrixType& B=modelHelper.getReducedOperator('N',j-offset,0,ModelHelperType::Environ);
				SparseMatrixType Aconj=modelHelper.getReducedOperator('C',i,0,ModelHelperType::System);
				SparseMatrixType Bconj=modelHelper.getReducedOperator('C',j-offset,0,ModelHelperType::Environ);
			
				// 0.5*S^+_i S^-_j
				FieldType angularFactor = -1.0;	
				modelHelper.fastOpProdInter(x,y,A,Bconj,type,valOver2,false,angularMomentum,angularFactor,2);
				
				// 0.5*S^-_i S^+_j
				angularFactor = -1.0;	
				modelHelper.fastOpProdInter(x,y,Aconj,B,type,valOver2,false,angularMomentum,angularFactor,0);
				
				// S^z_i*S^z_j
				const SparseMatrixType& Az=modelHelper.getReducedOperator('N',i,of,ModelHelperType::System);
				const SparseMatrixType& Bz=modelHelper.getReducedOperator('C',j-offset,of,ModelHelperType::Environ);
				angularFactor = 1.0/2.0; // needs to be in sync with LinkProductTjOneOrbital.h
				modelHelper.fastOpProdInter(x,y,Az,Bz,type,val,false,angularMomentum,angularFactor,1);
			} else {
				if (type!=EnvironSystem) std::cerr<<"EEEEEEEERRRRRRRRRRRROOOOOOOOOOORRRRRRRRRRR\n";

				const SparseMatrixType& A=modelHelper.getReducedOperator('N',i-offset,0,ModelHelperType::Environ);
				const SparseMatrixType& B=modelHelper.getReducedOperator('N',j,0,ModelHelperType::System);
				SparseMatrixType Aconj=modelHelper.getReducedOperator('C',i-offset,0,ModelHelperType::Environ);
				SparseMatrixType Bconj=modelHelper.getReducedOperator('C',j,0,ModelHelperType::System);
				
				// 0.5*S^+_i S^-_j	
				FieldType angularFactor = -1.0;
				modelHelper.fastOpProdInter(x,y,A,Bconj,type,valOver2,false,angularMomentum,angularFactor,2);
				
				// 0.5*S^-_i S^+_j
				angularFactor = -1.0;
				modelHelper.fastOpProdInter(x,y,Aconj,B,type,valOver2,false,angularMomentum,angularFactor,0);
				
				// S^z_i*S^z_j
				const SparseMatrixType& Az=modelHelper.getReducedOperator('N',i-offset,of,ModelHelperType::Environ);
				const SparseMatrixType& Bz=modelHelper.getReducedOperator('C',j,of,ModelHelperType::System);
				angularFactor = 1.0/2.0; // needs to be in sync with LinkProductTjOneOrbital.h
				modelHelper.fastOpProdInter(x,y,Az,Bz,type,val,false,angularMomentum,angularFactor,1);
			}
		}
		
		void linkProductHopping(std::vector<FieldType> &x,std::vector<FieldType> const &y,
					int i,int sigma,int j,int sigma2,int type,
			     FieldType  &val,
			     ModelHelperType const &modelHelper)  const
		{
			if (sigma!=sigma2) return;
			int const SystemEnviron=1,EnvironSystem=2;
			size_t of = 2;
			int offset = modelHelper.basis2().block().size();
			size_t angularMomentum=1;
			FieldType angularFactor  = 1.0;
			if (sigma==1) angularFactor  = -1.0;
			
			if (type==SystemEnviron) {
				
				//A=modelHelper.basis2().getOperator(i,sigma);
				//transposeConjugate(B,modelHelper.basis3().getOperator(j-offset,sigma));
				const SparseMatrixType& A=modelHelper.getReducedOperator('N',i,sigma+of,ModelHelperType::System);
				const SparseMatrixType& B=modelHelper.getReducedOperator('C',j-offset,sigma2+of,ModelHelperType::Environ);
				modelHelper.fastOpProdInter(x,y,A,B,type,val,true,angularMomentum,angularFactor,sigma);
						
			} else {
				if (type!=EnvironSystem) std::cerr<<"ERRRRRRRRRRORRRRRRRR\n";
					//A=modelHelper.basis3().getOperator(i-offset,sigma);
					//transposeConjugate(B,modelHelper.basis2().getOperator(j,sigma));
				const SparseMatrixType& A=modelHelper.getReducedOperator('N',i-offset,sigma+of,ModelHelperType::Environ);
				const SparseMatrixType& B=modelHelper.getReducedOperator('C',j,sigma2+of,ModelHelperType::System);
				modelHelper.fastOpProdInter(x,y,A,B,type,val,true,angularMomentum,angularFactor,sigma);
			}

			
		}
	}; // class LinkProductTjOneOrbital
} // namespace Dmrg
/*@}*/
#endif
