// Ghost node connectivity — translates connect_mesh, connect_orig_mesh, ghost_nodes.
// All indices in C++ data structures are 0-based.
// The Fortran code stores 1-based indices; we store 0-based throughout.

#include "fce/ghost_nodes.hpp"
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <cstring>

namespace fce {

// Helper: iperm arrays from Fortran
//   iperm(1,1:3) = {2,3,1}  ->  next_v[0]=1, next_v[1]=2, next_v[2]=0  (0-based vertex positions)
//   iperm(2,1:3) = {3,1,2}  ->  prev_v[0]=2, prev_v[1]=0, prev_v[2]=1
static const int next_v[3] = {1, 2, 0}; // iperm(1,:)-1
static const int prev_v[3] = {2, 0, 1}; // iperm(2,:)-1

// Find the 3 elements that share an edge with element ielem (0-based).
// neigh[k] stores the 1-based neighboring element index used by the Fortran code.
// Zero means boundary/no neighbor.
// mtable[node][0] = count, mtable[node][1..] = 1-based element indices.
static void find_elem_adj(int ielem, const TriElement& triang,
                          const std::vector<std::vector<int>>& mtable,
                          int neigh[3])
{
    neigh[0] = neigh[1] = neigh[2] = 0;

    for (int k = 0; k < 3; ++k) {
        if (triang.code_bc[k] == 1) continue; // boundary edge

        int iv1 = triang.vertices[k];
        int iv2 = triang.vertices[next_v[k]];

        // Find element that contains both iv1 and iv2 and is not ielem
        const auto& list1 = mtable[iv1];
        const auto& list2 = mtable[iv2];
        int cnt1 = list1[0];
        int cnt2 = list2[0];

        for (int a = 1; a <= cnt1; ++a) {
            int jelem = list1[a];
            if (jelem == ielem + 1) continue;
            for (int b = 1; b <= cnt2; ++b) {
                if (list2[b] == jelem) {
                    neigh[k] = jelem;
                    goto found;
                }
            }
            found:;
            if (neigh[k] != 0) break;
        }
    }
}

// Find local position (0,1,2) of global node iglob in triang vertices.
static int glob_loc(int iglob, const TriElement& triang)
{
    for (int i = 0; i < 3; ++i)
        if (triang.vertices[i] == iglob) return i;
    return -1; // should not happen
}

void connect_mesh(Mesh& meshh)
{
    int numnods = meshh.numnods;
    int numele  = meshh.numele;

    // mtable[node][0] = count; [node][1..] = 1-based element indices (max 6)
    const int MAXE = 15; // maxneigh_vert+1 in Fortran
    std::vector<std::vector<int>> mtable(numnods, std::vector<int>(MAXE, 0));

    // ntable equivalent (node adjacency) — we use it temporarily
    std::vector<std::vector<int>> ntable(numnods, std::vector<int>(MAXE, 0));

    // Build mtable and ntable
    for (int inode = 0; inode < numnods; ++inode) {
        for (int ielem = 0; ielem < numele; ++ielem) {
            const auto& el = meshh.connect[ielem];
            bool ask = false;
            for (int k = 0; k < 3; ++k)
                if (el.vertices[k] == inode) { ask = true; break; }

            if (ask) {
                int& cnt = mtable[inode][0];
                cnt++;
                mtable[inode][cnt] = ielem + 1;

                for (int k = 0; k < 3; ++k) {
                    int iv = el.vertices[k];
                    if (iv == inode) continue;
                    // Check if iv already in ntable[inode]
                    bool found = false;
                    int nc = ntable[inode][0];
                    for (int m = 1; m <= nc; ++m)
                        if (ntable[inode][m] == iv) { found = true; break; }
                    if (!found) {
                        ntable[inode][0]++;
                        ntable[inode][ntable[inode][0]] = iv;
                        ntable[iv][0]++;
                        ntable[iv][ntable[iv][0]] = inode;
                    }
                }
            }
        }
    }

    // Fill neigh_elem and neigh_vert for each element
    for (int ielem = 0; ielem < numele; ++ielem) {
        TriElement& triang = meshh.connect[ielem];
        triang.neigh_elem.fill(0);
        triang.neigh_vert.fill(0);
        int neigh[3];
        find_elem_adj(ielem, triang, mtable, neigh);

        const int* ivert = triang.vertices.data();

        // ── Edge 1 (vertex 0) traversal ──────────────────────────────
        if (neigh[0] != 0) {
            // Right of edge 1
            // ilist_el = {3,2,1,12} → indices 2,1,0,11 (0-based)
            // ilist_ve = {3,1,2, 5} → indices 2,0,1, 4
            {
                int ilist_el[] = {2, 1, 0, 11};
                int ilist_ve[] = {2, 0, 1,  4};
                int ipivot = ivert[0];
                int master = ielem + 1;
                TriElement tri_master = meshh.connect[master - 1];
                int i_master = glob_loc(ipivot, tri_master);
                int istep = 0;
                for (;;) {
                    int neigh2[3];
                    find_elem_adj(master - 1, tri_master, mtable, neigh2);
                    int iact   = i_master;
                    int mslave = neigh2[iact];
                    bool flag = (mslave == 0) || (mslave == neigh[2]);
                    if (flag) break;
                    TriElement tri_slave = meshh.connect[mslave - 1];
                    int i_slave = glob_loc(ipivot, tri_slave);
                    triang.neigh_elem[ilist_el[istep]] = mslave;
                    triang.neigh_vert[ilist_ve[istep]] = tri_slave.vertices[next_v[i_slave]] + 1;
                    master = mslave;
                    tri_master = tri_slave;
                    i_master = i_slave;
                    istep++;
                    if (istep >= 3) break;
                }
            }

            // Left of edge 1
            // ilist_el = {4,5,6,0} → 3,4,5,-1
            // ilist_ve = {6,10,11,0} → 5,9,10,-1
            if (triang.neigh_elem[2] != 0) {
                int ilist_el[] = {3, 4, 5, -1};
                int ilist_ve[] = {5, 9, 10, -1};
                int ipivot = ivert[1];
                int master = triang.neigh_elem[2]; // neigh_elem(3) in Fortran = index 2
                TriElement tri_master = meshh.connect[master - 1];
                int i_master = glob_loc(ipivot, tri_master);
                int istep = 0;
                for (;;) {
                    int neigh2[3];
                    find_elem_adj(master - 1, tri_master, mtable, neigh2);
                    int iact   = prev_v[i_master];
                    int mslave = neigh2[iact];
                    bool flag = (mslave == 0) || (mslave == neigh[1]);
                    if (flag) break;
                    TriElement tri_slave = meshh.connect[mslave - 1];
                    int i_slave = glob_loc(ipivot, tri_slave);
                    triang.neigh_elem[ilist_el[istep]] = mslave;
                    triang.neigh_vert[ilist_ve[istep]] = tri_slave.vertices[prev_v[i_slave]] + 1;
                    master = mslave;
                    tri_master = tri_slave;
                    i_master = i_slave;
                    istep++;
                    if (istep >= 3) break;
                }
            }
        }

        // ── Edge 2 (vertex 1) traversal ──────────────────────────────
        if (neigh[1] != 0) {
            // Right of edge 2
            // ilist_el = {7,6,5,4} → 6,5,4,3
            // ilist_ve = {11,10,6,3} → 10,9,5,2
            {
                int ilist_el[] = {6, 5, 4, 3};
                int ilist_ve[] = {10, 9, 5, 2};
                int ipivot = ivert[1];
                int master = ielem + 1;
                TriElement tri_master = meshh.connect[master - 1];
                int i_master = glob_loc(ipivot, tri_master);
                int istep = 0;
                for (;;) {
                    int neigh2[3];
                    find_elem_adj(master - 1, tri_master, mtable, neigh2);
                    int iact   = i_master;
                    int mslave = neigh2[iact];
                    bool flag = (mslave == 0) ||
                                (triang.neigh_elem[ilist_el[istep]] != 0);
                    if (flag) break;
                    TriElement tri_slave = meshh.connect[mslave - 1];
                    int i_slave = glob_loc(ipivot, tri_slave);
                    triang.neigh_elem[ilist_el[istep]] = mslave;
                    triang.neigh_vert[ilist_ve[istep]] = tri_slave.vertices[next_v[i_slave]] + 1;
                    master = mslave;
                    tri_master = tri_slave;
                    i_master = i_slave;
                    istep++;
                    if (istep >= 3) break;
                }
            }

            // Left of edge 2
            // ilist_el = {8,9,10,0} → 7,8,9,-1
            // ilist_ve = {12,9,5,0} → 11,8,4,-1
            if (triang.neigh_elem[6] != 0) {
                int ilist_el[] = {7, 8, 9, -1};
                int ilist_ve[] = {11, 8, 4, -1};
                int ipivot = ivert[2];
                int master = triang.neigh_elem[6];
                TriElement tri_master = meshh.connect[master - 1];
                int i_master = glob_loc(ipivot, tri_master);
                int istep = 0;
                for (;;) {
                    int neigh2[3];
                    find_elem_adj(master - 1, tri_master, mtable, neigh2);
                    int iact   = prev_v[i_master];
                    int mslave = neigh2[iact];
                    bool flag = (mslave == 0) || (mslave == neigh[2]);
                    if (flag) break;
                    TriElement tri_slave = meshh.connect[mslave - 1];
                    int i_slave = glob_loc(ipivot, tri_slave);
                    triang.neigh_elem[ilist_el[istep]] = mslave;
                    triang.neigh_vert[ilist_ve[istep]] = tri_slave.vertices[prev_v[i_slave]] + 1;
                    master = mslave;
                    tri_master = tri_slave;
                    i_master = i_slave;
                    istep++;
                    if (istep >= 3) break;
                }
            }
        }

        // ── Edge 3 (vertex 2) traversal ──────────────────────────────
        if (neigh[2] != 0) {
            // Right of edge 3
            // ilist_el = {11,10,9,8} → 10,9,8,7
            // ilist_ve = {5,9,12,11} → 4,8,11,10
            {
                int ilist_el[] = {10, 9, 8, 7};
                int ilist_ve[] = {4, 8, 11, 10};
                int ipivot = ivert[2];
                int master = ielem + 1;
                TriElement tri_master = meshh.connect[master - 1];
                int i_master = glob_loc(ipivot, tri_master);
                int istep = 0;
                for (;;) {
                    int neigh2[3];
                    find_elem_adj(master - 1, tri_master, mtable, neigh2);
                    int iact   = i_master;
                    int mslave = neigh2[iact];
                    bool flag = (mslave == 0) ||
                                (triang.neigh_elem[ilist_el[istep]] != 0);
                    if (flag) break;
                    TriElement tri_slave = meshh.connect[mslave - 1];
                    int i_slave = glob_loc(ipivot, tri_slave);
                    triang.neigh_elem[ilist_el[istep]] = mslave;
                    triang.neigh_vert[ilist_ve[istep]] = tri_slave.vertices[next_v[i_slave]] + 1;
                    master = mslave;
                    tri_master = tri_slave;
                    i_master = i_slave;
                    istep++;
                    if (istep >= 3) break;
                }
            }

            // Left of edge 3
            // ilist_el = {12,1,2,0} → 11,0,1,-1
            // ilist_ve = {2,1,3,0} → 1,0,2,-1
            if (triang.neigh_elem[10] != 0) {
                int ilist_el[] = {11, 0, 1, -1};
                int ilist_ve[] = {1, 0, 2, -1};
                int ipivot = ivert[0];
                int master = triang.neigh_elem[10];
                TriElement tri_master = meshh.connect[master - 1];
                int i_master = glob_loc(ipivot, tri_master);
                int istep = 0;
                for (;;) {
                    int neigh2[3];
                    find_elem_adj(master - 1, tri_master, mtable, neigh2);
                    int iact   = prev_v[i_master];
                    int mslave = neigh2[iact];
                    bool flag = (mslave == 0) || (mslave == neigh[0]);
                    if (flag) break;
                    TriElement tri_slave = meshh.connect[mslave - 1];
                    int i_slave = glob_loc(ipivot, tri_slave);
                    triang.neigh_elem[ilist_el[istep]] = mslave;
                    triang.neigh_vert[ilist_ve[istep]] = tri_slave.vertices[prev_v[i_slave]] + 1;
                    master = mslave;
                    tri_master = tri_slave;
                    i_master = i_slave;
                    istep++;
                    if (istep >= 3) break;
                }
            }
        }

        // Self-vertices at fixed positions (Fortran lines 229-231, 1-based → 0-based)
        triang.neigh_vert[3]  = triang.vertices[0] + 1;
        triang.neigh_vert[6]  = triang.vertices[1] + 1;
        triang.neigh_vert[7]  = triang.vertices[2] + 1;

        triang.num_neigh_elem = 0;
        triang.num_neigh_vert = 0;
        for (int k = 0; k < 12; ++k) {
            if (triang.neigh_elem[k] != 0) triang.num_neigh_elem++;
            if (triang.neigh_vert[k] != 0) triang.num_neigh_vert++;
        }
    }
}

void connect_orig_mesh(Mesh& mesh0, const Mesh& meshg, int ncol, int nrow)
{
    int numno  = (nrow + 1) * (ncol + 1);
    int numnog = (nrow + 3) * (ncol + 3);
    int numghost = 2 * nrow + 2 * ncol + 6;

    // nelemfg[ielem_0based] = jelem_0based in ghost mesh
    // nelem2g[jelem_0based] = ielem_0based in orig mesh (0 = no entry)
    // We use 0 as "no entry" for nelem2g, so we store actual index+1...
    // Actually Fortran uses 0 as "no entry" for nelem2g since elements are 1-based.
    // We use -1 for "no entry".
    std::vector<int> nelemfg(mesh0.numele, -1);
    std::vector<int> nelem2g(meshg.numele, -1);

    for (int irow = 1; irow <= nrow; ++irow) {
        for (int icol = 1; icol <= 2 * ncol; ++icol) {
            int ielem = (irow - 1) * ncol * 2 + icol; // 1-based
            int jelem = irow * 2 * (ncol + 2) + 2 + icol; // 1-based
            nelemfg[ielem - 1] = jelem - 1;
            nelem2g[jelem - 1] = ielem - 1;
        }
    }

    // node2g: node2g[jnode_0based][0] = type (0=none, 1=real, 2=ghost)
    //         node2g[jnode_0based][1] = index (0-based real or 0-based ghost)
    std::vector<std::array<int,2>> node2g(numnog, {0, -1});

    // Real nodes
    for (int irow = 1; irow <= nrow + 1; ++irow) {
        for (int icol = 1; icol <= ncol + 1; ++icol) {
            int inode = (irow - 1) * (ncol + 1) + icol; // 1-based
            int jnode = irow * (ncol + 3) + 1 + icol;   // 1-based
            node2g[jnode - 1][0] = 1;
            node2g[jnode - 1][1] = inode - 1; // 0-based
        }
    }

    // Ghost nodes — mirror of Fortran ighost counter
    int ighost = 0;

    // First row of ghost (irow=1 boundary, icol=1..ncol+2)
    for (int icol = 1; icol <= ncol + 2; ++icol) {
        int jnode = icol + 1; // 1-based in extended grid first row
        node2g[jnode - 1][0] = 2;
        node2g[jnode - 1][1] = ighost; // 0-based ghost index
        ighost++;
    }

    // Left and right columns (irow=2..nrow+2)
    for (int irow = 2; irow <= nrow + 2; ++irow) {
        int inode_l = (irow - 1) * (ncol + 3) + 1; // 1-based left
        node2g[inode_l - 1][0] = 2;
        node2g[inode_l - 1][1] = ighost;
        ighost++;

        int inode_r = irow * (ncol + 3); // 1-based right
        node2g[inode_r - 1][0] = 2;
        node2g[inode_r - 1][1] = ighost;
        ighost++;
    }

    // Last row of ghost (irow=nrow+3, icol=1..ncol+2)
    for (int icol = 1; icol <= ncol + 2; ++icol) {
        int jnode = (nrow + 3 - 1) * (ncol + 3) + icol; // 1-based
        node2g[jnode - 1][0] = 2;
        node2g[jnode - 1][1] = ighost;
        ighost++;
    }

    // Map neigh_vert and neigh_elem from ghost mesh to original mesh
    for (int ielem = 0; ielem < mesh0.numele; ++ielem) {
        TriElement& triang = mesh0.connect[ielem];
        int jelem = nelemfg[ielem];
        const TriElement& triangg = meshg.connect[jelem];

        for (int jj = 0; jj < 12; ++jj) {
            int gv = triangg.neigh_vert[jj];
            if (gv <= 0 || gv > numnog) {
                triang.neigh_vert[jj] = -1;
                continue;
            }
            int type = node2g[gv - 1][0];
            int idx  = node2g[gv - 1][1];
            if (type == 1) {
                triang.neigh_vert[jj] = idx; // 0-based real node
            } else if (type == 2) {
                triang.neigh_vert[jj] = numno + idx; // 0-based ghost = numnods + ghost_index
            } else {
                triang.neigh_vert[jj] = -1;
            }
        }

        for (int jj = 0; jj < 12; ++jj) {
            int ge = triangg.neigh_elem[jj];
            if (ge <= 0) {
                triang.neigh_elem[jj] = 0;
            } else {
                int re = nelem2g[ge - 1];
                triang.neigh_elem[jj] = (re >= 0) ? (re + 1) : 0;
            }
        }

        triang.num_neigh_elem = 0;
        triang.num_neigh_vert = 0;
        for (int k = 0; k < 12; ++k) {
            if (triang.neigh_elem[k] != 0) triang.num_neigh_elem++;
            if (triang.neigh_vert[k] >= 0) triang.num_neigh_vert++;
        }
    }

    // Build nghost_tab for mesh0
    // numghost = 2*(nrow + ncol) + 6
    // Fortran uses 1-based jghost; we use 0-based
    int nedge = numghost;
    mesh0.nedge = nedge;
    mesh0.nghost_tab.assign(nedge, {0, 0, 0});

    // iperm arrays
    // In Fortran: iperm(1,1:3)={2,3,1}, iperm(2,1:3)={3,1,2}
    // ied = 1 means: i1=ied=1, i2=iperm(1,1)=2, i3=iperm(2,1)=3 (1-based local vertex)
    // In 0-based: i1=0, i2=1, i3=2
    // ied = 3 means: i1=2, i2=iperm(1,3)-1=0, i3=iperm(2,3)-1=1

    // First row (bottom): ied=1 → i1=0, i2=1, i3=2
    {
        int jghost = 1; // 1-based in Fortran
        for (int icol = 0; icol < 2*ncol; icol += 2) { // Type1 elements of bottom row
            int ielem = icol; // 0-based element, odd type1 elements: 0,2,4,...,2*(ncol-1)
            const auto& el = mesh0.connect[ielem];
            jghost++;
            mesh0.nghost_tab[jghost - 1] = {el.vertices[0], el.vertices[1], el.vertices[2]};
        }
    }

    // First col (left): ied=3 → i1=2, i2=next_v[2]=0, i3=prev_v[2]=1
    {
        int jghost = ncol + 3; // 1-based
        for (int irow = 0; irow < nrow; ++irow) {
            int ielem = irow * ncol * 2; // 0-based, first type1 element of each row
            const auto& el = mesh0.connect[ielem];
            jghost += 2;
            mesh0.nghost_tab[jghost - 1] = {el.vertices[2], el.vertices[0], el.vertices[1]};
        }
    }

    // Last col (right): ied=3 → i1=2, i2=next_v[2]=0, i3=prev_v[2]=1
    {
        int jghost = ncol + 2; // 1-based
        for (int irow = 0; irow < nrow; ++irow) {
            int ielem = irow * ncol * 2 + ncol * 2 - 1; // 0-based last type2 element of row
            const auto& el = mesh0.connect[ielem];
            jghost += 2;
            mesh0.nghost_tab[jghost - 1] = {el.vertices[2], el.vertices[0], el.vertices[1]};
        }
    }

    // Last row (top): ied=1 → i1=0, i2=1, i3=2
    {
        int jghost = ncol + 2 + 2*(nrow + 1) + 1; // 1-based
        int irow_start = (nrow - 1) * 2 * ncol + 2 - 1; // 0-based, Fortran: (nrow-1)*2*ncol+2
        for (int icol = 0; icol < ncol; ++icol) {
            int ielem = irow_start + icol * 2; // type2 elements of top row
            const auto& el = mesh0.connect[ielem];
            jghost++;
            mesh0.nghost_tab[jghost - 1] = {el.vertices[0], el.vertices[1], el.vertices[2]};
        }
    }

    // Corners (6 special ghost nodes)
    // Corner 1: jghost=1
    {
        const auto& el = mesh0.connect[0];
        // Fortran: nghost_tab(1,:) = [ivert(1), ivert(1), ivert(3)]
        mesh0.nghost_tab[0] = {el.vertices[0], el.vertices[0], el.vertices[2]};
    }

    // Corner 2: jghost=ncol+3
    {
        const auto& el = mesh0.connect[0];
        // Fortran: nghost_tab(ncol+3,:) = [ivert(1), ivert(1), ivert(2)]
        mesh0.nghost_tab[ncol + 3 - 1] = {el.vertices[0], el.vertices[0], el.vertices[1]};
    }

    // Corner 3: jghost=ncol+2
    // Fortran: nghost_tab(ncol+2,:) = [numnods+ncol+1, ncol+1, ncol]
    // 1-based → 0-based: [numnods+ncol, ncol, ncol-1]
    {
        mesh0.nghost_tab[ncol + 2 - 1] = {numno + ncol, ncol, ncol - 1};
    }

    // Corner 4: jghost=ncol+2+2*(nrow+1)+1
    // Fortran: nghost_tab(jg,:) = [(ncol+1)*nrow+1, numnods+jghost-2, (ncol+1)*(nrow-1)+1]
    // 0-based: [(ncol+1)*nrow, numnods+jg-2, (ncol+1)*(nrow-1)]  where jg is 1-based
    {
        int jg = ncol + 2 + 2*(nrow + 1) + 1; // 1-based
        mesh0.nghost_tab[jg - 1] = {(ncol + 1) * nrow, numno + jg - 3, (ncol + 1) * (nrow - 1)};
    }

    // Corner 5: jghost=(ncol+2)+2*(nrow+1)
    // Fortran: nghost_tab(jg,:) = [numnods, numnods, numnods-1]
    // 0-based: [numno-1, numno-1, numno-2]
    {
        int jg = (ncol + 2) + 2*(nrow + 1); // 1-based
        mesh0.nghost_tab[jg - 1] = {numno - 1, numno - 1, numno - 2};
    }

    // Corner 6: jghost=2*(ncol+2)+2*(nrow+1)
    // Fortran: nghost_tab(jg,:) = [numnods, numnods, numnods-ncol-1]
    // 0-based: [numno-1, numno-1, numno-ncol-2]
    {
        int jg = 2*(ncol + 2) + 2*(nrow + 1); // 1-based
        mesh0.nghost_tab[jg - 1] = {numno - 1, numno - 1, numno - ncol - 2};
    }
}

void ghost_nodes(const Mesh& meshh, FlatCoords& x0)
{
    // Fortran: do ijk=1,meshh%nedge
    //   i1=nghost_tab(ijk,1), i2=nghost_tab(ijk,2), i3=nghost_tab(ijk,3)  (1-based)
    //   ghost = x(i1) + x(i2) - x(i3)
    //   x(numnods+ijk) = ghost   (1-based)
    // In C++ (0-based): x0[(numnods+ijk)*3+k]
    int numnods = meshh.numnods;
    int nedge   = meshh.nedge;

    // Ensure x0 is large enough
    x0.resize(3 * (numnods + nedge), 0.0);

    for (int ijk = 0; ijk < nedge; ++ijk) {
        int i1 = meshh.nghost_tab[ijk][0]; // 0-based
        int i2 = meshh.nghost_tab[ijk][1];
        int i3 = meshh.nghost_tab[ijk][2];
        for (int k = 0; k < 3; ++k) {
            x0[(numnods + ijk) * 3 + k] = x0[i1*3+k] + x0[i2*3+k] - x0[i3*3+k];
        }
    }
}

} // namespace fce
