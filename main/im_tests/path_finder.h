//
//  for astar visualize
//	2023-03-03 / im dong ye
//
//	TODO list:
//
//

#ifndef __path_finder_h_
#define __path_finder_h_

#include <queue>
#include <glm/glm.hpp>

namespace lim
{
	class PathFinder
	{
	public:
		enum NodeState : int
		{
			NS_ROAD, NS_OPEN, NS_CLOSE, NS_WALL, NS_START, NS_END, NS_PATH
		};
		// (4+4)+8(x64)+4+4+4+(4padding) = 32byte
		struct Node
		{
			glm::ivec2 pos;
			Node *prev;
			int gCost, hCost;
			NodeState state;
		};
		// fCost = gCost+hCost
		struct CompareNodePointer
		{
			bool operator()(Node *a, Node *b)
			{
				int afCost = a->gCost + a->hCost;

				int bfCost = b->gCost + b->hCost;

				if( afCost > bfCost ) return true;
				// 같은 fCost사이에서 h가 작은값을 우선순위로 둔다.
				if( afCost == bfCost && a->hCost > b->hCost ) return true;
				return false;
			}
		};
		// 왜인지 모르겠는데 priority_queue는 begin, end가 구현안되어있다.
		// !node pointer저장하는 nodeMap으로 이제 탐색 안함. 필요없는코드
		struct NodePPQ: public std::priority_queue<Node*, std::vector<Node*>, CompareNodePointer>
		{
		public:
			container_type::iterator begin() { return priority_queue::c.begin(); }
			container_type::iterator end() { return priority_queue::c.end(); }
		};

	public:
		/*********** global valriables ************/
		int width, height;
		std::vector<std::vector<Node>> map; // 1000*1000*1 = 1Mb
		Node __wall, __space;
		glm::ivec2 start_pos, end_pos; // (goal)
		glm::ivec2 dir[4] ={
				{1,0}, //우
				{0,1}, //하
				{-1,0},//좌
				{0,-1},//상
		};
		std::vector<glm::ivec2> path;
	public:
		PathFinder(int _width, int _height)
		{
			resizeMap(_width, _height);
		}
		void resizeMap(int _width, int _height)
		{
			width = _width; height = _height;
			start_pos ={-1,-1};
			end_pos ={-1,-1};
			map.resize(height);
			for( int i=0; i<height; i++ ) {
				map[i].resize(width);
				for( int j=0; j<width; j++ ) {
					map[i][j].pos ={j, i};
					map[i][j].prev = nullptr;
					map[i][j].state = NS_ROAD;
				}
			}
		}
		void clearMap()
		{
			int nrStart=0;
			int nrEnd=0;
			for( int i=0; i<height; i++ ) for( int j=0; j<width; j++ ) {
				switch( map[i][j].state ) {
					case NS_OPEN:
					case NS_CLOSE:
					case NS_PATH:
						map[i][j].state = NS_ROAD;
						break;
					case NS_START:
						nrStart++;
						break;
					case NS_END:
						nrEnd++;
						break;
					case NS_WALL:
					case NS_ROAD:
						break;
				}
			}
			if( nrStart!=1||nrEnd!=1 ) {
				printf("error there one more param");
			}
		}
		bool setMapPos(int x, int y, NodeState ns) // // 0:space, 1:wall, 2:start, 3:dest
		{
			if( x<0||y<0||x>=width||y>=height ) return false;

			glm::ivec2 oldStartPos, oldEndPos;
			NodeState prevNs = map[y][x].state;

			if( prevNs==NS_START ) {
				start_pos ={-1,-1};
			}
			if( prevNs==NS_END ) {
				end_pos ={-1,-1};
			}

			if( ns==NS_START ) {
				oldStartPos = start_pos;
				start_pos ={x, y};
				if( oldStartPos != start_pos && oldStartPos.x>=0 ) {
					map[oldStartPos.y][oldStartPos.x].state = NS_ROAD;
				}
			}
			else if( ns==NS_END ) {
				oldEndPos = end_pos;
				end_pos ={x, y};
				if( oldEndPos != end_pos && oldEndPos.x>=0 ) {
					map[oldEndPos.y][oldEndPos.x].state = NS_ROAD;
				}
			}
			
			map[y][x].state = ns;

			if( start_pos!=end_pos && start_pos.x>=0 && end_pos.x>=0 ) {
				updatePath();
			}
			return true;
		}
	private:		
		// admissible heuristic fuction으로 실제 거리보다 같거나 짧은거리를 사용한다.
		int manhattan(glm::ivec2& a, glm::ivec2& b)
		{
			int xDist = glm::abs(a.x-b.x);
			int yDist = glm::abs(a.y-b.y);
			return xDist+yDist;
		}
	public:
		/****** A star maze path finder ******/
		// From : https://ko.wikipedia.org/wiki/A*_%EC%95%8C%EA%B3%A0%EB%A6%AC%EC%A6%98
		// And	  https://www.youtube.com/watch?v=-L-WgKMFuhE
		int updatePath()
		{
			clearMap();
			NodePPQ open;
			map[start_pos.y][start_pos.x].gCost = 0;
			map[start_pos.y][start_pos.x].hCost = manhattan(start_pos, end_pos);
			map[start_pos.y][start_pos.x].prev = nullptr;
			map[start_pos.y][start_pos.x].state = NS_OPEN;

			open.push(&map[start_pos.y][start_pos.x]);
			Node *cur=nullptr;
			while( !open.empty() ) {
				// best first search
				cur = open.top(); open.pop();

				if( cur->state == NS_END ) break;

				cur->state = NS_CLOSE;

				int gCost = cur->gCost+1;

				for( int i=0; i<4; i++ ) {
					glm::ivec2 newPos = cur->pos + dir[i];
					if( newPos.x<0||newPos.y<0||newPos.x>=width||newPos.y>=height )
						continue;

					Node& nextNode = map[newPos.y][newPos.x];

					if( nextNode.state==NS_ROAD ) {
						nextNode.gCost = gCost;
						nextNode.hCost = manhattan(newPos, end_pos);
						nextNode.prev = cur;
						nextNode.state = NS_OPEN;
						open.push(&nextNode);
						continue;
					}
					// open 되어있으면 새로운 path보다 큰지 비교후 교체
					if( nextNode.state==NS_OPEN && nextNode.gCost>gCost ) {
						nextNode.gCost = gCost;
						nextNode.prev = cur;
						continue;
					}
					if( nextNode.state==NS_END ) {
						nextNode.gCost = 0;
						nextNode.hCost = 0;
						nextNode.prev = cur;
						open.push(&nextNode);
						break;
					}
				}
			}
			
			path.resize(0);
			if( cur==nullptr || !(cur->pos==end_pos) ) {
				printf("[error] no path\n");
				return -1;
			}
			path.reserve(width*3);
			Node* backtrack = cur;
			while( backtrack!=nullptr ) {
				backtrack->state = NS_PATH;
				path.push_back(cur->pos);
				backtrack = backtrack->prev;
			}
			reverse(path.begin(), path.end());
			map[start_pos.y][start_pos.x].state = NS_START;
			map[end_pos.y][end_pos.x].state = NS_END;
			// if you want print?
			return path.size();
		}
	};
}
#endif