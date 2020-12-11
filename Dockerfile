FROM alpine AS building

WORKDIR /root
ADD . /root/source

RUN apk add build-base autoconf automake curl-dev json-c-dev && \
	cd source && \
	sh ./autogen.sh && \
	mkdir ../build && \
	./configure --prefix="/root/build/" && \
	make && \
	make install

FROM alpine AS runtime
WORKDIR /
COPY --from=0 /root/build/ ./usr/local/
RUN apk add --no-cache curl json-c
ENTRYPOINT [ "/usr/local/bin/twtfwd" ]
