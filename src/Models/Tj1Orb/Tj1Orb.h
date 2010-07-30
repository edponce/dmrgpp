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

/*! \file Tj1Orb.h
 *
 *  Hubbard + Heisenberg
 *
 */
#ifndef TJ_1ORB_H
#define TJ_1ORB_H
#include "ModelHubbard.h"
#include "ModelHeisenberg.h"
#include "LinkProdTj1Orb.h"

namespace Dmrg {
	//! t-J model for DMRG solver, uses ModelHubbard and ModelHeisenberg by containment
	template<typename ModelHelperType_,
	typename SparseMatrixType,
	typename DmrgGeometryType,
	template<typename> class SharedMemoryTemplate>
	class Tj1Orb : public ModelBase<ModelHelperType_,SparseMatrixType,DmrgGeometryType,
 		LinkProdTj1Orb<ModelHelperType_>,SharedMemoryTemplate> {
		public:
			typedef ModelHubbard<ModelHelperType_,SparseMatrixType,DmrgGeometryType,
 			SharedMemoryTemplate> ModelHubbardType;
			typedef ModelHeisenberg<ModelHelperType_,SparseMatrixType,DmrgGeometryType,
 			SharedMemoryTemplate> ModelHeisenbergType;
			typedef ModelHelperType_ ModelHelperType;
			typedef typename ModelHelperType::OperatorsType OperatorsType;
			typedef typename OperatorsType::OperatorType OperatorType;
			typedef typename ModelHelperType::RealType RealType;
			typedef typename SparseMatrixType::value_type SparseElementType;

	public:
		typedef LinkProdTj1Orb<ModelHelperType> LinkProductType;
		typedef ModelBase<ModelHelperType,SparseMatrixType,DmrgGeometryType,LinkProductType,SharedMemoryTemplate> ModelBaseType;
		typedef	typename ModelBaseType::MyBasis MyBasis;
		typedef	typename ModelBaseType::MyBasisWithOperators MyBasisWithOperators;
		typedef typename MyBasis::BasisDataType BasisDataType;
		typedef typename ModelHubbardType::HilbertBasisType HilbertBasisType;
		typedef typename ModelHelperType::BlockType Block;
		typedef typename ModelHubbardType::HilbertSpaceHubbardType HilbertSpaceHubbardType;
		
		Tj1Orb(ParametersModelHubbard<RealType> const &mp,DmrgGeometryType const &dmrgGeometry) 
			: ModelBaseType(dmrgGeometry),modelParameters_(mp), dmrgGeometry_(dmrgGeometry),
				modelHubbard_(mp,dmrgGeometry)
		{
		}

		size_t orbitals() const { return modelHubbard_.orbitals(); }

		//! find creation operator matrices for (i,sigma) in the natural basis, find quantum numbers and number of electrons
		//! for each state in the basis
		void setNaturalBasis(std::vector<OperatorType> &creationMatrix,SparseMatrixType &hamiltonian,
				BasisDataType &q,Block const &block) const
		{
			
			modelHubbard_.setNaturalBasis(creationMatrix,hamiltonian,q,block);

			// add ni to creationMatrix
			setNi(creationMatrix,block);

			// add V_{ij} n_i n_j to hamiltonian
			addNiNj(hamiltonian,creationMatrix,block);
		}

		//! set creation matrices for sites in block
		void setOperatorMatrices(std::vector<OperatorType> &creationMatrix,Block const &block) const
		{
			modelHubbard_.setOperatorMatrices(creationMatrix,block);
			// add ni to creationMatrix
			setNi(creationMatrix,block);
		}

		psimag::Matrix<SparseElementType> getOperator(const std::string& what,size_t gamma=0,size_t spin=0) const
		{
			Block block;
			block.resize(1);
			block[0]=0;
			std::vector<OperatorType> creationMatrix;
			setOperatorMatrices(creationMatrix,block);

			if (what=="n") {
				psimag::Matrix<SparseElementType> tmp;
				crsMatrixToFullMatrix(tmp,creationMatrix[2].data);
				return tmp;
			} else {
				return modelHubbard_.getOperator(what,gamma,spin);
			}
		}
		
		//! find total number of electrons for each state in the basis
		void findElectrons(std::vector<size_t> &electrons,
				   std::vector<typename HilbertSpaceHubbardType::HilbertState>  const &basis) const
		{
			modelHubbard_.findElectrons(electrons,basis);
		}

		//! find all states in the natural basis for a block of n sites
		//! N.B.: HAS BEEN CHANGED TO ACCOMODATE FOR MULTIPLE BANDS
		void setNaturalBasis(HilbertBasisType  &basis,int n) const
		{
			modelHubbard_.setNaturalBasis(basis,n);
		}
		
		void print(std::ostream& os) const
		{
			modelHubbard_.print(os);
		}

	private:
		const ParametersModelHubbard<RealType>&  modelParameters_;
		const DmrgGeometryType &dmrgGeometry_;
		ModelHubbardType modelHubbard_;

		//! Find n_i in the natural basis natBasis
		SparseMatrixType findOperatorMatrices(int i,
			std::vector<typename HilbertSpaceHubbardType::HilbertState> const &natBasis) const
		{
			
			size_t n = natBasis.size();
			psimag::Matrix<typename SparseMatrixType::value_type> cm(n,n);
			
			for (size_t ii=0;ii<natBasis.size();ii++) {
				typename HilbertSpaceHubbardType::HilbertState ket=natBasis[ii];
				for (size_t sigma=0;sigma<2;sigma++) 
					if (HilbertSpaceHubbardType::isNonZero(ket,i,sigma)) 
						cm(ii,ii) += 1.0;
			}

			SparseMatrixType creationMatrix(cm);
			return creationMatrix;
		}


		//! Full hamiltonian from creation matrices cm
		void addNiNj(SparseMatrixType &hmatrix,std::vector<OperatorType> const &cm,Block const &block) const
		{
			//Assume block.size()==1 and then problem solved!! there are no connection if there's only one site ;-)
			assert(block.size()==1);
// 			for (size_t sigma=0;sigma<DEGREES_OF_FREEDOM;sigma++) 
// 				for (size_t sigma2=0;sigma2<DEGREES_OF_FREEDOM;sigma2++) 
// 					addNiNj(hmatrix,cm,block,sigma,sigma2);
		}
		
		void setNi(std::vector<OperatorType> &creationMatrix,Block const &block) const
		{
			assert(block.size()==1);
			std::vector<typename HilbertSpaceHubbardType::HilbertState> natBasis;

			modelHubbard_.setNaturalBasis(natBasis,block.size());
			
			SparseMatrixType tmpMatrix = findOperatorMatrices(0,natBasis);
			RealType angularFactor= 1;
			typename OperatorType::Su2RelatedType su2related;
			su2related.offset = 1; //check FIXME
			OperatorType myOp(tmpMatrix,1,typename OperatorType::PairType(0,0),angularFactor,su2related);
					
			creationMatrix.push_back(myOp);
		}
	};	//class ExtendedHubbard1Orb

	template<typename ModelHelperType,
	typename SparseMatrixType,
	typename DmrgGeometryType,
	template<typename> class SharedMemoryTemplate>
	std::ostream &operator<<(std::ostream &os,
		const ExtendedHubbard1Orb<ModelHelperType,SparseMatrixType,DmrgGeometryType,SharedMemoryTemplate>& model)
	{
		model.print(os);
		return os;
	}
} // namespace Dmrg
/*@}*/
#endif // TJ_1ORB_H
