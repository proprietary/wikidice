import type { Metadata } from "next";
import { Inter } from "next/font/google";
import "./globals.css";

const inter = Inter({ subsets: ["latin"] });

export const metadata: Metadata = {
  title: "Wikidice",
  description: "Lookup random Wikipedia articles by category. Wikidice searches recursively below the category you provide.",
  authors: [{ name: "Zelly Snyder <zelcon@zelcon.net>", url: "https://www.zelcon.net/", },],
  icons: {
    // rel="icon"
    icon: [{
      url: "/favicon.ico",
    }, {
      url: "/favicon-16x16.png",
      sizes: "16x16",
      type: "image/png",
    }, {
      url: "/favicon-32x32.png",
      sizes: "32x32",
      type: "image/png",
    }, {
      url: "/logo.png",
      sizes: "1024x1024",
      type: "image/png",
    }],
    // rel="apple-touch-icon"
    apple: {
      url: "/apple-touch-icon.png",
      sizes: "180x180",
      type: "image/png",
    },
  }
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en">
      <body className={inter.className}>{children}</body>
    </html>
  );
}
