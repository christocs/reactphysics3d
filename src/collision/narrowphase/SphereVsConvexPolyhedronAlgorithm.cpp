/********************************************************************************
* ReactPhysics3D physics library, http://www.reactphysics3d.com                 *
* Copyright (c) 2010-2018 Daniel Chappuis                                       *
*********************************************************************************
*                                                                               *
* This software is provided 'as-is', without any express or implied warranty.   *
* In no event will the authors be held liable for any damages arising from the  *
* use of this software.                                                         *
*                                                                               *
* Permission is granted to anyone to use this software for any purpose,         *
* including commercial applications, and to alter it and redistribute it        *
* freely, subject to the following restrictions:                                *
*                                                                               *
* 1. The origin of this software must not be misrepresented; you must not claim *
*    that you wrote the original software. If you use this software in a        *
*    product, an acknowledgment in the product documentation would be           *
*    appreciated but is not required.                                           *
*                                                                               *
* 2. Altered source versions must be plainly marked as such, and must not be    *
*    misrepresented as being the original software.                             *
*                                                                               *
* 3. This notice may not be removed or altered from any source distribution.    *
*                                                                               *
********************************************************************************/

// Libraries
#include "SphereVsConvexPolyhedronAlgorithm.h"
#include "GJK/GJKAlgorithm.h"
#include "SAT/SATAlgorithm.h"
#include "collision/NarrowPhaseInfoBatch.h"

// We want to use the ReactPhysics3D namespace
using namespace reactphysics3d;

// Compute the narrow-phase collision detection between a sphere and a convex polyhedron
// This technique is based on the "Robust Contact Creation for Physics Simulations" presentation
// by Dirk Gregorius.
void SphereVsConvexPolyhedronAlgorithm::testCollision(NarrowPhaseInfoBatch& narrowPhaseInfoBatch, uint batchStartIndex, uint batchNbItems,
                                                      bool reportContacts, MemoryAllocator& memoryAllocator) {

    // First, we run the GJK algorithm
    GJKAlgorithm gjkAlgorithm;

#ifdef IS_PROFILING_ACTIVE

        gjkAlgorithm.setProfiler(mProfiler);

#endif

    List<GJKAlgorithm::GJKResult> gjkResults(memoryAllocator);
    gjkAlgorithm.testCollision(narrowPhaseInfoBatch, batchStartIndex, batchNbItems, gjkResults, reportContacts);

    // For each item in the batch
    uint resultIndex=0;
    for (uint batchIndex = batchStartIndex; batchIndex < batchStartIndex + batchNbItems; batchIndex++) {

        assert(!narrowPhaseInfoBatch.isColliding[batchIndex]);

        assert(narrowPhaseInfoBatch.collisionShapes1[batchIndex]->getType() == CollisionShapeType::CONVEX_POLYHEDRON ||
            narrowPhaseInfoBatch.collisionShapes2[batchIndex]->getType() == CollisionShapeType::CONVEX_POLYHEDRON);
        assert(narrowPhaseInfoBatch.collisionShapes1[batchIndex]->getType() == CollisionShapeType::SPHERE ||
            narrowPhaseInfoBatch.collisionShapes2[batchIndex]->getType() == CollisionShapeType::SPHERE);

        // Get the last frame collision info
        LastFrameCollisionInfo* lastFrameCollisionInfo = narrowPhaseInfoBatch.getLastFrameCollisionInfo(batchIndex);

        lastFrameCollisionInfo->wasUsingGJK = true;
        lastFrameCollisionInfo->wasUsingSAT = false;

        // If we have found a contact point inside the margins (shallow penetration)
        if (gjkResults[resultIndex] == GJKAlgorithm::GJKResult::COLLIDE_IN_MARGIN) {

            // Return true
            narrowPhaseInfoBatch.isColliding[batchIndex] = true;
            continue;
        }

        // If we have overlap even without the margins (deep penetration)
        if (gjkResults[resultIndex] == GJKAlgorithm::GJKResult::INTERPENETRATE) {

            // Run the SAT algorithm to find the separating axis and compute contact point
            SATAlgorithm satAlgorithm(memoryAllocator);

#ifdef IS_PROFILING_ACTIVE

            satAlgorithm.setProfiler(mProfiler);

#endif

            satAlgorithm.testCollisionSphereVsConvexPolyhedron(narrowPhaseInfoBatch, batchIndex, 1, reportContacts);

            lastFrameCollisionInfo->wasUsingGJK = false;
            lastFrameCollisionInfo->wasUsingSAT = true;

            continue;
        }

        resultIndex++;
    }
}
