# Color aware viewer

## meta data
이미지 포맷 바다 metadata 저장을 위한 필드가 있다.
JPEG: APP1 segment
PNG : eXIf chunk
WebP: EXIF chunk
metadata의 포맷에는 아래의 3개가 있다.

### Exif( exchangeable image file format )
카메라는 대부분 JPEG/Exif 로 저장됨
이 포맷은 TIFF 의 Tag구조를 따라했다.
독립적인 포맷이 아니라 JPEG, RIFF, WAV, PNG1.2(2017) 와 같이 사용된다.
- 날짜, 시간
- 카메라 정보
- 세팅 ( 초점거리, 스트로보, 조리개, 셔터속도 )
- 회전 정보
- 위치 정보
- 파일탐색기 등에 보여질 썸네일 ( 수정할때 꼭 바꿔줄 것 )
- 이미지 설명
- 저작권

### XMP( extemsible metadata platform )
어도비에서 iso규격에 맞춘 포맷. 이미지, 동영상, 오디오, 문서(html) 에서 사용.
data model과 serialization으로 파트가 나눠지는데
serialization은 data model을 표현한다.
data model:
	