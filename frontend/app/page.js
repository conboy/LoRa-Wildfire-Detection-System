"use client"

import Navbar from "@/components/Navbar";
import dynamic from 'next/dynamic'


export default function Home() {
  const Map = dynamic(
    () => import('@/components/Map'),
    { ssr: false }
  )

  return (
    <main className="flex flex-col h-screen">
      <div className="flex-shrink-0">
        <Navbar />
      </div>
      <div className="flex-grow">
        <Map />
      </div>
  </main>
  );
}
